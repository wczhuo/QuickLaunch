#include "Config.h"
#include <fstream>
#include <windows.h>

// 极简内联 JSON 解析：只解析 [ { "key": "value", ... }, ... ] 格式
// 用于 QuickLaunch 这种固定结构配置文件

static std::string jsonValue(const std::string& s, const char* key) {
    char keyBuf[64];
    snprintf(keyBuf, sizeof(keyBuf), "\"%s\"", key);
    size_t kp = s.find(keyBuf);
    if (kp == std::string::npos) return "";
    size_t vp = s.find(':', kp);
    if (vp == std::string::npos) return "";
    vp++;
    // 跳过空白
    while (vp < s.size() && (s[vp] == ' ' || s[vp] == '\t' || s[vp] == '\n')) vp++;
    if (vp >= s.size()) return "";
    if (s[vp] == '"') {
        // 字符串值
        vp++;
        size_t end = vp;
        while (end < s.size() && s[end] != '"') {
            if (s[end] == '\\' && end + 1 < s.size()) end++;
            end++;
        }
        return s.substr(vp, end - vp);
    }
    return "";
}

static std::wstring toWstr(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (len == 0) return L"";
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], len);
    return ws;
}

static std::string wstrToUtf8(const std::wstring& ws) {
    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    if (len == 0) return "";
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], len, NULL, NULL);
    return s;
}

bool Config::load(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        baseDir_ = path.substr(0, pos + 1);
    } else {
        baseDir_ = L".\\";
    }

    // 读取文件（UTF-8）
    std::string utf8Path = wstrToUtf8(path);
    std::ifstream file(utf8Path, std::ios::binary);
    if (!file) return false;
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // 找第一个 '[' 和最后一个 ']'
    size_t start = content.find('[');
    size_t end = content.find_last_of(']');
    if (start == std::string::npos || end == std::string::npos) return false;
    content = content.substr(start + 1, end - start - 1);

    // 逐个解析对象 { ... }
    size_t pos2 = 0;
    while (pos2 < content.size()) {
        size_t objStart = content.find('{', pos2);
        if (objStart == std::string::npos) break;
        size_t objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string obj = content.substr(objStart, objEnd - objStart + 1);
        LaunchItem li;
        li.name  = toWstr(jsonValue(obj, "name"));
        li.icon  = toWstr(jsonValue(obj, "icon"));
        li.path  = toWstr(jsonValue(obj, "path"));
        if (!li.name.empty()) items_.push_back(li);

        pos2 = objEnd + 1;
    }

    return !items_.empty();
}
