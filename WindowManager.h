#pragma once
#include "ButtonRenderer.h"
#include <windows.h>

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    bool init(const wchar_t* configPath);
    int run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void createWindow();
    void render();
    void refreshWindow();
    void expand();
    void collapse();
    void animateTo(int targetY);
    void startHideTimer();
    void cancelHideTimer();
    void launchItem(const std::wstring& path);

    HWND  hwnd_      = NULL;
    HDC   screenDC_  = NULL;
    HDC   memDC_     = NULL;
    HBITMAP memBmp_  = NULL;

    int barWidth_  = 800;
    int barHeight_ = 50;
    int screenWidth_  = 0;
    int screenHeight_ = 0;

    int visibleY_   = 0;
    int collapsedY_ = 0;
    int expandedY_  = 0;

    bool isExpanded_  = false;
    bool isAnimating_ = false;
    int  animTargetY_  = 0;
    int  animCurrentY_ = 0;

    UINT hideTimerId_ = 0;
    UINT animTimerId_ = 0;

    ButtonRenderer renderer_;
    bool mouseInWindow_ = false;
    int  hoverBtnIndex_ = -1;
};
