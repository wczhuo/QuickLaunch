#include "WindowManager.h"
#include <windows.h>
#include <wtypes.h>
#include <gdiplus.h>
#include <string>

#pragma comment(lib, "gdiplus.lib")

static std::wstring getExePath() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::wstring(buf);
}

static void trim(std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) { s.clear(); return; }
    size_t b = s.find_last_not_of(L" \t\r\n");
    s = s.substr(a, b - a + 1);
}

static bool setAutostart(bool enable) {
    HKEY hKey;
    LONG r = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE | (enable ? KEY_QUERY_VALUE : 0), &hKey);
    if (r != ERROR_SUCCESS) {
        // Try opening with just KEY_SET_VALUE on delete
        if (!enable) {
            r = RegOpenKeyExW(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                0, KEY_SET_VALUE, &hKey);
        }
        if (r != ERROR_SUCCESS) return false;
    }

    if (!enable) {
        RegDeleteValueW(hKey, L"QuickLaunch");
        RegCloseKey(hKey);
        return true;
    }

    std::wstring exe = getExePath();
    RegSetValueExW(hKey, L"QuickLaunch", 0, REG_SZ,
                   (const BYTE*)exe.c_str(), (DWORD)((exe.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    return true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

    // 解析命令行参数
    std::wstring cmd = L"";
    {
        int argc = 0;
        wchar_t* argv[10] = { nullptr };
        wchar_t* cmdLine = GetCommandLineW();
        // 简单解析（处理引号）
        wchar_t* p = cmdLine;
        while (*p && argc < 10) {
            while (*p == L' ') ++p;
            if (*p == L'"') { ++p; wchar_t* q = p; while (*q && *q != L'"') ++q; *q = 0; argv[argc++] = p; p = q + 1; }
            else { wchar_t* q = p; while (*q && *q != L' ') ++q; *q = 0; if (*p) argv[argc++] = p; p = q + 1; }
        }
        // 从 argv[1] 开始是参数
        for (int i = 1; i < argc; ++i) {
            std::wstring arg(argv[i]);
            if (arg == L"--autostart" && i + 1 < argc) {
                cmd = argv[i + 1];
                ++i;
            } else if (arg == L"--autostart") {
                cmd = L"on";
            }
        }
        trim(cmd);
    }

    if (!cmd.empty()) {
        bool on = (cmd == L"on");
        bool off = (cmd == L"off");
        if (on || off) {
            bool ok = setAutostart(on);
            std::wstring msg = ok
                ? (on ? L"已开启开机自启" : L"已关闭开机自启")
                : (on ? L"开启开机自启失败" : L"关闭开机自启失败");
            MessageBoxW(NULL, msg.c_str(), L"QuickLaunch", MB_ICONINFORMATION | MB_OK);
            return 0;
        }
    }

    // GDI+ 初始化
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    gdiplusStartupInput.GdiplusVersion = 1;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 获取 exe 同目录下的 Config.json
    std::wstring cfgPath = getExePath();
    size_t p2 = cfgPath.find_last_of(L"\\/");
    if (p2 != std::wstring::npos) cfgPath = cfgPath.substr(0, p2 + 1);
    cfgPath += L"Config.json";

    WindowManager wm;
    if (!wm.init(cfgPath.c_str())) {
        MessageBoxW(NULL, L"加载配置失败", L"QuickLaunch Error", MB_ICONERROR);
        return 1;
    }

    int ret = wm.run();

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return ret;
}
