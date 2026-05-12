#include "ButtonRenderer.h"
#include <windows.h>
#include <gdiplus.h>
#include <string>

static bool isAbsPath(const wchar_t* p) {
    if (!p || !p[0]) return false;
    return (p[1] == L':') || (p[0] == L'\\' && p[1] == L'\\') || (p[0] == L'/');
}

static void fillRoundRect(Gdiplus::Graphics& g, const Gdiplus::Brush& br, int x, int y, int w, int h, int r) {
    if (w <= 0 || h <= 0 || r <= 0) return;
    int d = r * 2;
    Gdiplus::GraphicsPath path;
    path.AddLine(x + r, y,       x + w - r, y);
    path.AddArc(x + w - d, y,    d, d, -90, 90);
    path.AddLine(x + w, y + r,   x + w, y + h - r);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddLine(x + w - r, y + h, x + r, y + h);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.AddLine(x, y + h - r, x, y + r);
    path.AddArc(x, y, d, d, 180, 90);
    path.CloseFigure();
    g.FillPath(&br, &path);
}

static const Gdiplus::Color BG_COLOR(255, 0x2d, 0x2d, 0x2d);
static const Gdiplus::Color BTN_NORMAL(255, 0x3d, 0x3d, 0x3d);
static const Gdiplus::Color BTN_HOVER(255, 0x4d, 0x4d, 0x4d);
static const Gdiplus::Color TEXT_COLOR(255, 0xff, 0xff, 0xff);

ButtonRenderer::ButtonRenderer() {}
ButtonRenderer::~ButtonRenderer() {
    for (auto& b : buttons_) delete b.iconImg;
    delete closeBtn_.iconImg;
}

void ButtonRenderer::setItems(const std::vector<LaunchItem>& items, const std::wstring& baseDir) {
    for (auto& b : buttons_) delete b.iconImg;
    buttons_.clear();
    baseDir_ = baseDir;

    for (const auto& item : items) {
        Button btn;
        btn.name = item.name;
        btn.iconPath = item.icon;
        if (!isAbsPath(btn.iconPath.c_str()))
            btn.iconPath = baseDir + btn.iconPath;
        btn.launchPath = item.path;
        if (!isAbsPath(btn.launchPath.c_str()))
            btn.launchPath = baseDir + btn.launchPath;
        buttons_.push_back(btn);
    }
    loadIcons();
    calcLayout();
}

void ButtonRenderer::setCloseIcon(const std::wstring& path) {
    std::wstring fullPath = path;
    if (!isAbsPath(fullPath.c_str())) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring exeDir = exePath;
        size_t p2 = exeDir.find_last_of(L"\\/");
        if (p2 != std::wstring::npos) exeDir = exeDir.substr(0, p2 + 1);
        fullPath = exeDir + L"close.png";
    }
    delete closeBtn_.iconImg;
    closeBtn_.iconImg = new Gdiplus::Image(fullPath.c_str());
    closeBtn_.loaded = (closeBtn_.iconImg->GetLastStatus() == Gdiplus::Ok);
    if (closeBtn_.loaded) {
        closeBtn_.iconW = closeBtn_.iconImg->GetWidth();
        closeBtn_.iconH = closeBtn_.iconImg->GetHeight();
        if (closeBtn_.iconW > 20) {
            closeBtn_.iconH = 20 * closeBtn_.iconH / closeBtn_.iconW;
            closeBtn_.iconW = 20;
        }
        if (closeBtn_.iconH > 20) {
            closeBtn_.iconW = 20 * closeBtn_.iconW / closeBtn_.iconH;
            closeBtn_.iconH = 20;
        }
        closeBtn_.w = closeBtn_.iconW + 4;
        closeBtn_.h = closeBtn_.iconH + 4;
    }
}

void ButtonRenderer::loadIcons() {
    for (auto& btn : buttons_) {
        delete btn.iconImg;
        btn.iconImg = nullptr;
        btn.iconW = btn.iconH = 0;
        if (btn.iconPath.empty()) continue;
        Gdiplus::Image* img = new Gdiplus::Image(btn.iconPath.c_str());
        if (img->GetLastStatus() != Gdiplus::Ok) { delete img; continue; }
        btn.iconImg = img;
        btn.iconW = img->GetWidth();
        btn.iconH = img->GetHeight();
        if (btn.iconW > 26) {
            btn.iconH = 26 * btn.iconH / btn.iconW;
            btn.iconW = 26;
        }
        if (btn.iconH > 26) {
            btn.iconW = 26 * btn.iconW / btn.iconH;
            btn.iconH = 26;
        }
    }
}

void ButtonRenderer::calcLayout() {
    barHeight_ = 50;

    int N = (int)buttons_.size();
    if (N == 0) { barWidth_ = 100; return; }

    HDC hdc = GetDC(NULL);
    Gdiplus::Graphics gg(hdc);
    gg.SetTextRenderingHint(Gdiplus::TextRenderingHintSystemDefault);
    Gdiplus::Font font(L"Microsoft YaHei", 10, Gdiplus::FontStyleRegular);
    Gdiplus::PointF origin(0, 0);

    int gap = 10;
    int btnH = 26;
    int iconPadding = 6;
    int textPadding = 20;
    int closeAreaW = 60;
    int leftMargin = 10;

    int totalBtnW = 0;
    for (int i = 0; i < N; ++i) {
        Gdiplus::RectF bb;
        gg.MeasureString(buttons_[i].name.c_str(), -1, &font, origin, &bb);
        int textW = (int)bb.Width;
        int iconSpace = (buttons_[i].iconW > 0) ? buttons_[i].iconW + iconPadding : 0;
        buttons_[i].w = iconSpace + textW + textPadding;
        buttons_[i].h = btnH;
        totalBtnW += buttons_[i].w;
        if (i > 0) totalBtnW += gap;
    }
    ReleaseDC(NULL, hdc);

    barWidth_ = leftMargin + totalBtnW + closeAreaW + 10;

    int closeW = closeBtn_.loaded ? (closeBtn_.iconW + 4) : 24;
    int closeH = closeBtn_.loaded ? (closeBtn_.iconH + 4) : 24;
    closeBtn_.w = closeW;
    closeBtn_.h = closeH;
    closeBtn_.x = barWidth_ - 8 - closeW;
    closeBtn_.y = (barHeight_ - closeH) / 2;

    int contentW = barWidth_ - leftMargin - closeAreaW - 10;
    int btnStartX = leftMargin + (contentW - totalBtnW) / 2;
    int x = btnStartX;
    for (int i = 0; i < N; ++i) {
        buttons_[i].x = x;
        buttons_[i].y = (barHeight_ - btnH) / 2;
        x += buttons_[i].w + gap;
    }
}

int ButtonRenderer::hitTest(int x, int y) const {
    if (y < 0 || y > barHeight_) return -1;
    if (x >= closeBtn_.x && x <= closeBtn_.x + closeBtn_.w &&
        y >= closeBtn_.y && y <= closeBtn_.y + closeBtn_.h) return -2;
    for (int i = 0; i < (int)buttons_.size(); ++i) {
        const auto& b = buttons_[i];
        if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) return i;
    }
    return -1;
}

std::wstring ButtonRenderer::triggerButton(int index) {
    if (index < 0 || index >= (int)buttons_.size()) return L"";
    return buttons_[index].launchPath;
}

void ButtonRenderer::draw(Gdiplus::Graphics& g, int hoverIndex) {
    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    // 工具栏背景（圆角）
    Gdiplus::SolidBrush bgBrush(BG_COLOR);
    fillRoundRect(g, bgBrush, 0, 0, barWidth_, barHeight_, barRadius_);

    // 按钮（圆角）
    Gdiplus::Font font(L"Microsoft YaHei", 10, Gdiplus::FontStyleRegular);
    Gdiplus::SolidBrush textBrush(TEXT_COLOR);
    for (int i = 0; i < (int)buttons_.size(); ++i) {
        const Button& btn = buttons_[i];
        bool hover = (i == hoverIndex);
        Gdiplus::Color fillCol = hover ? BTN_HOVER : BTN_NORMAL;
        Gdiplus::SolidBrush btnBrush(fillCol);
        fillRoundRect(g, btnBrush, btn.x, btn.y, btn.w, btn.h, 6);

        // 图标
        if (btn.iconImg) {
            int ix = btn.x + 8;
            int iy = btn.y + (btn.h - btn.iconH) / 2;
            g.DrawImage(btn.iconImg, ix, iy, btn.iconW, btn.iconH);
        }

        // 文字（垂直居中）
        int textX = btn.x + (btn.iconW > 0 ? btn.iconW + 14 : 8);
        int textW = btn.w - (textX - btn.x) - 8;
        if (textW > 10) {
            Gdiplus::RectF rc((Gdiplus::REAL)textX, (Gdiplus::REAL)btn.y,
                               (Gdiplus::REAL)textW, (Gdiplus::REAL)btn.h);
            Gdiplus::StringFormat sf;
            sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            sf.SetAlignment(Gdiplus::StringAlignmentNear);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            sf.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
            g.DrawString(btn.name.c_str(), -1, &font, rc, &sf, &textBrush);
        }
    }

    // 关闭按钮
    if (closeBtn_.loaded && closeBtn_.iconImg) {
        int cx = closeBtn_.x + (closeBtn_.w - closeBtn_.iconW) / 2;
        int cy = closeBtn_.y + (closeBtn_.h - closeBtn_.iconH) / 2;
        g.DrawImage(closeBtn_.iconImg, cx, cy, closeBtn_.iconW, closeBtn_.iconH);
    } else {
        Gdiplus::Font font2(L"Arial", 16, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush xBrush(TEXT_COLOR);
        Gdiplus::RectF rc((Gdiplus::REAL)closeBtn_.x, (Gdiplus::REAL)closeBtn_.y,
                           (Gdiplus::REAL)closeBtn_.w, (Gdiplus::REAL)closeBtn_.h);
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(L"\xD7", 1, &font2, rc, &sf, &xBrush);
    }
}
