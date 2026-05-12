#include "WindowManager.h"
#include <windows.h>
#include <wtypes.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    // GDI+ 初始化
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    gdiplusStartupInput.GdiplusVersion = 1;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 获取 exe 同目录下的 Config.json
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring cfgPath = exePath;
    size_t p = cfgPath.find_last_of(L"\\/");
    if (p != std::wstring::npos) cfgPath = cfgPath.substr(0, p + 1);
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
