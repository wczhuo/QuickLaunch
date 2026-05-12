#include "WindowManager.h"
#include <windowsx.h>
#include <shellapi.h>
#include <wtypes.h>
#include <gdiplus.h>

static const int ANIM_STEP = 6;
static const int ANIM_INTERVAL = 8;
static const int HIDE_DELAY = 300;
static const int TRACK_MARGIN = 3;

WindowManager::WindowManager() {}

WindowManager::~WindowManager() {
    if (memBmp_) DeleteObject(memBmp_);
    if (memDC_)  DeleteDC(memDC_);
    if (screenDC_) ReleaseDC(NULL, screenDC_);
}

bool WindowManager::init(const wchar_t* configPath) {
    Config cfg;
    if (!cfg.load(configPath)) return false;

    renderer_.setItems(cfg.items(), cfg.baseDir());

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t p2 = exeDir.find_last_of(L"\\/");
    if (p2 != std::wstring::npos) exeDir = exeDir.substr(0, p2 + 1);
    renderer_.setCloseIcon(exeDir + L"close.png");

    barWidth_  = renderer_.barWidth();
    barHeight_ = renderer_.barHeight();
    screenWidth_  = GetSystemMetrics(SM_CXSCREEN);
    screenHeight_ = GetSystemMetrics(SM_CYSCREEN);

    collapsedY_ = -barHeight_ + TRACK_MARGIN;
    expandedY_   = 0;
    visibleY_    = collapsedY_;   // 初始收起

    createWindow();
    return true;
}

void WindowManager::createWindow() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = GetModuleHandle(NULL);
    wc.lpszClassName = L"QuickLaunch_WndClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    int winX = (screenWidth_ - barWidth_) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"QuickLaunch",
        WS_POPUP,
        winX, visibleY_, barWidth_, barHeight_,
        NULL, NULL, wc.hInstance, this
    );
    if (!hwnd_) return;

    screenDC_ = GetDC(NULL);
    memDC_    = CreateCompatibleDC(screenDC_);
    memBmp_   = CreateCompatibleBitmap(screenDC_, barWidth_, barHeight_);
    SelectObject(memDC_, memBmp_);

    render();
    refreshWindow();

    SetWindowPos(hwnd_, HWND_TOPMOST, winX, visibleY_, 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void WindowManager::render() {
    Gdiplus::Graphics g(memDC_);
    renderer_.draw(g, hoverBtnIndex_);
}

void WindowManager::refreshWindow() {
    SIZE  sz  = { barWidth_, barHeight_ };
    POINT pt  = { 0, 0 };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hwnd_, screenDC_, NULL, &sz, memDC_, &pt, 0, &bf, ULW_ALPHA);
}

void WindowManager::expand() {
    if (isExpanded_) return;
    isExpanded_ = true;
    cancelHideTimer();
    animateTo(expandedY_);
}

void WindowManager::collapse() {
    if (!isExpanded_) return;
    isExpanded_ = false;
    animateTo(collapsedY_);
}

void WindowManager::animateTo(int targetY) {
    if (isAnimating_) { animTargetY_ = targetY; return; }
    animTargetY_  = targetY;
    animCurrentY_ = visibleY_;
    isAnimating_  = true;
    animTimerId_  = SetTimer(hwnd_, 2, ANIM_INTERVAL, NULL);
}

void WindowManager::startHideTimer() {
    cancelHideTimer();
    hideTimerId_ = SetTimer(hwnd_, 1, HIDE_DELAY, NULL);
}

void WindowManager::cancelHideTimer() {
    if (hideTimerId_) { KillTimer(hwnd_, hideTimerId_); hideTimerId_ = 0; }
}

void WindowManager::launchItem(const std::wstring& path) {
    if (path.empty()) return;
    ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

int WindowManager::run() {
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

LRESULT WindowManager::HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TIMER: {
            if (wParam == 2) {
                int delta = animTargetY_ - animCurrentY_;
                if (abs(delta) <= ANIM_STEP) {
                    KillTimer(hwnd, animTimerId_);
                    animTimerId_ = 0;
                    visibleY_    = animTargetY_;
                    isAnimating_  = false;
                } else {
                    visibleY_    += (delta > 0 ? ANIM_STEP : -ANIM_STEP);
                    animCurrentY_ = visibleY_;
                }
                int winX = (screenWidth_ - barWidth_) / 2;
                SetWindowPos(hwnd, 0, winX, visibleY_, 0, 0,
                             SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            } else if (wParam == 1) {
                KillTimer(hwnd, hideTimerId_);
                hideTimerId_ = 0;
                if (!mouseInWindow_) collapse();
            }
            break;
        }
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            if (!mouseInWindow_) {
                mouseInWindow_ = true;
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                expand();
            }
            int newHover = renderer_.hitTest(x, y);
            if (newHover != hoverBtnIndex_) {
                hoverBtnIndex_ = newHover;
                render();
                refreshWindow();
            }
            if (isExpanded_) startHideTimer();
            break;
        }
        case WM_MOUSELEAVE: {
            mouseInWindow_ = false;
            hoverBtnIndex_ = -1;
            render();
            refreshWindow();
            startHideTimer();
            break;
        }
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int idx = renderer_.hitTest(x, y);
            if (idx == -2) {
                PostQuitMessage(0);
            } else if (idx >= 0) {
                launchItem(renderer_.triggerButton(idx));
            }
            break;
        }
        case WM_NCHITTEST: return HTCLIENT;
        case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowManager* self = NULL;
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        self = (WindowManager*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (WindowManager*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }
    if (self) return self->HandleMsg(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
