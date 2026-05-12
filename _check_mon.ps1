Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public class Mon {
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    public static extern bool EnumDisplayMonitors(IntPtr hdc, IntPtr rc, MonitorEnumProc del, IntPtr data);

    public delegate bool MonitorEnumProc(IntPtr hMonitor, IntPtr hdc, ref RECT rc, IntPtr data);

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    public static extern bool GetMonitorInfo(IntPtr hMonitor, ref MONITORINFO mi);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left, Top, Right, Bottom;
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
    public class MONITORINFO {
        public int cbSize;
        public RECT rcMonitor;
        public RECT rcWork;
        public int dwFlags;
        public MONITORINFO() { cbSize = Marshal.SizeOf(typeof(MONITORINFO)); }
    }
}

public class W2 {
    [DllImport("user32.dll")]
    public static extern IntPtr FindWindow(string c, string n);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr h, out Mon.RECT r);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr h);
}

"@

$monitors = @()
$cb = [Mon+MonitorEnumProc]{
    param($h, $dc, [ref]$r, [ref]$p)
    $mi = New-Object Mon+MONITORINFO
    [Mon]::GetMonitorInfo($h, [ref]$mi) | Out-Null
    $monitors += [PSCustomObject]@{
        Left=$mi.rcMonitor.Left
        Top=$mi.rcMonitor.Top
        Right=$mi.rcMonitor.Right
        Bottom=$mi.rcMonitor.Bottom
        WorkLeft=$mi.rcWork.Left
        WorkTop=$mi.rcWork.Top
        WorkRight=$mi.rcWork.Right
        WorkBottom=$mi.rcWork.Bottom
    }
    return $true
}

[Mon]::EnumDisplayMonitors([IntPtr]::Zero, [IntPtr]::Zero, $cb, [IntPtr]::Zero) | Out-Null

Write-Host "=== Monitors ==="
$monitors | ForEach-Object {
    Write-Host "Monitor: [$($_.Left),$($_.Top)] size=$($_.Right-$_.Left)x$($_.Bottom-$_.Top)"
    Write-Host "Work:    [$($_.WorkLeft),$($_.WorkTop)] size=$($_.WorkRight-$_.WorkLeft)x$($_.WorkBottom-$_.WorkTop)"
}

$h = [W2]::FindWindow("QuickLaunch_WndClass", "QuickLaunch")
if ($h -ne [IntPtr]::Zero) {
    $r = New-Object Mon+RECT
    [W2]::GetWindowRect($h, [ref]$r) | Out-Null
    $vis = [W2]::IsWindowVisible($h)
    Write-Host "`n=== QuickLaunch Window ==="
    Write-Host "HWND=$h  Visible=$vis"
    Write-Host "Rect=($($r.Left),$($r.Top)) -> ($($r.Right),$($r.Bottom)) W=$($r.Right-$r.Left) H=$($r.Bottom-$r.Top)"
} else {
    Write-Host "`nWindow not found!"
}
