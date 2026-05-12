#pragma once
#include "Config.h"
#include <windows.h>
#include <wtypes.h>
#include <gdiplus.h>
#include <vector>

class ButtonRenderer {
public:
    ButtonRenderer();
    ~ButtonRenderer();

    void setItems(const std::vector<LaunchItem>& items, const std::wstring& baseDir);
    void setCloseIcon(const std::wstring& path);

    int hitTest(int x, int y) const;
    void draw(Gdiplus::Graphics& g, int hoverIndex);

    int barWidth()  const { return barWidth_; }
    int barHeight() const { return barHeight_; }
    std::wstring triggerButton(int index);

private:
    void loadIcons();
    void calcLayout();

    struct Button {
        std::wstring name;
        std::wstring iconPath;
        std::wstring launchPath;
        int x = 0, y = 0, w = 0, h = 0;
        int iconW = 0, iconH = 0;
        Gdiplus::Image* iconImg = nullptr;
    };

    struct CloseBtn {
        int x = 0, y = 0, w = 24, h = 24;
        int iconW = 0, iconH = 0;
        Gdiplus::Image* iconImg = nullptr;
        bool loaded = false;
    };

    CloseBtn closeBtn_;
    std::vector<Button> buttons_;
    std::wstring baseDir_;

    int barWidth_  = 800;
    int barHeight_ = 50;
    int barRadius_ = 24;
};
