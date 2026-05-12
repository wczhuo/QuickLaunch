#pragma once
#include <string>
#include <vector>

struct LaunchItem {
    std::wstring name;
    std::wstring icon;
    std::wstring path;
};

class Config {
public:
    bool load(const std::wstring& path);

    const std::vector<LaunchItem>& items() const { return items_; }
    const std::wstring& baseDir() const { return baseDir_; }

private:
    std::vector<LaunchItem> items_;
    std::wstring baseDir_;
};
