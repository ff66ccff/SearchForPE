#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <cwchar>
#include <cstring>

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

// ---------------------------------------------------------------- geometry
struct RectF {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

    RectF() = default;
    RectF(float xx, float yy, float ww, float hh) : x(xx), y(yy), w(ww), h(hh) {}

    float Right() const { return x + w; }
    float Bottom() const { return y + h; }
    float CenterX() const { return x + w * 0.5f; }
    float CenterY() const { return y + h * 0.5f; }

    bool Contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    RectF Inset(float dx, float dy) const {
        return RectF(x + dx, y + dy, (std::max)(0.0f, w - dx * 2.0f), (std::max)(0.0f, h - dy * 2.0f));
    }
    RectF Offset(float dx, float dy) const { return RectF(x + dx, y + dy, w, h); }
    RectF WithY(float yy) const { return RectF(x, yy, w, h); }

    D2D1_RECT_F Raw() const { D2D1_RECT_F r = { x, y, x + w, y + h }; return r; }
};

inline float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

// ---------------------------------------------------------------- color
using Color = D2D1_COLOR_F;

inline Color Hex(UINT32 rgb, float alpha = 1.0f) {
    Color c;
    c.r = ((rgb >> 16) & 0xFF) / 255.0f;
    c.g = ((rgb >> 8) & 0xFF) / 255.0f;
    c.b = (rgb & 0xFF) / 255.0f;
    c.a = alpha;
    return c;
}
inline Color WithAlpha(Color c, float alpha) { c.a = alpha; return c; }
inline COLORREF ToColorRef(Color c) {
    return RGB((int)(c.r * 255.0f + 0.5f), (int)(c.g * 255.0f + 0.5f), (int)(c.b * 255.0f + 0.5f));
}

// ---------------------------------------------------------------- data
struct Question {
    int id = 0;
    std::wstring issue;
    std::vector<std::wstring> options;
    std::wstring answer;
    std::wstring type;              // 判断题 / 单选题 / 多选题
    std::wstring issue_lower;
    std::wstring options_lower_combined;

    bool IsJudge() const { return options.empty(); }
    bool IsMulti() const { return type == L"多选题"; }
    bool IsTrueAnswer() const { return answer == L"对" || answer == L"正确"; }

    // Human readable answer, e.g. "正确" / "错误" / "A" / "B·C"
    std::wstring AnswerText() const {
        if (IsJudge()) return IsTrueAnswer() ? L"正确" : L"错误";
        std::wstring compact;
        for (wchar_t ch2 : answer) {
            if (ch2 == L' ' || ch2 == L',' || ch2 == L'，' || ch2 == L'、') continue;
            if (!compact.empty()) compact.push_back(L'\u00B7');
            compact.push_back(ch2);
        }
        return compact.empty() ? L"未知" : compact;
    }
    // Full plain-text payload used by the copy action.
    std::wstring CopyText() const {
        std::wstring out = L"【" + type + L"】" + issue + L"\r\n";
        for (const auto& option : options) out += option + L"\r\n";
        out += L"正确答案：" + AnswerText();
        return out;
    }
};

// ---------------------------------------------------------------- string utils
inline std::wstring Utf8ToWide(const char* utf8, int len = -1) {
    if (!utf8 || len == 0) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, len, NULL, 0);
    if (wlen <= 0) return L"";
    std::wstring result(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8, len, &result[0], wlen);
    if (len == -1 && !result.empty() && result.back() == L'\0') result.pop_back();
    return result;
}

inline std::string WideToUtf8(const wchar_t* wide, int len = -1) {
    if (!wide || len == 0) return "";
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide, len, NULL, 0, NULL, NULL);
    if (ulen <= 0) return "";
    std::string result(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide, len, &result[0], ulen, NULL, NULL);
    if (len == -1 && !result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

inline std::wstring ToLower(const std::wstring& str) {
    std::wstring lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return lower;
}

inline std::wstring FormatInt(long long value) {
    wchar_t buf[32];
    swprintf(buf, 32, L"%lld", value);
    std::wstring digits(buf);
    bool negative = !digits.empty() && digits[0] == L'-';
    if (negative) digits.erase(digits.begin());
    std::wstring out;
    int count = 0;
    for (int i = (int)digits.size() - 1; i >= 0; --i) {
        out.push_back(digits[i]);
        if (++count % 3 == 0 && i > 0) out.push_back(L',');
    }
    if (negative) out.push_back(L'-');
    std::reverse(out.begin(), out.end());
    return out;
}

inline std::wstring FormatMs(double ms) {
    wchar_t buf[64];
    if (ms < 10.0) swprintf(buf, 64, L"%.2f ms", ms);
    else if (ms < 100.0) swprintf(buf, 64, L"%.1f ms", ms);
    else swprintf(buf, 64, L"%.0f ms", ms);
    return buf;
}

// ---------------------------------------------------------------- clipboard
inline bool CopyToClipboard(HWND hwnd, const std::wstring& text) {
    if (!OpenClipboard(hwnd)) return false;
    EmptyClipboard();
    size_t bytes = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hMem) {
        CloseClipboard();
        return false;
    }
    void* dst = GlobalLock(hMem);
    memcpy(dst, text.c_str(), bytes);
    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return true;
}

// ---------------------------------------------------------------- dpi
inline int ScaleI(int value, UINT dpi) { return MulDiv(value, (int)dpi, 96); }
inline float ScaleF(float value, UINT dpi) { return value * (float)dpi / 96.0f; }
inline float PixelsToDip(int px, UINT dpi) { return (float)px * 96.0f / (float)dpi; }
inline int DipToPixels(float dip, UINT dpi) { return (int)std::lround(dip * (float)dpi / 96.0f); }

// ---------------------------------------------------------------- imgui-ish ids
inline UINT32 PackColorKey(Color c) {
    UINT32 r = (UINT32)(Clampf(c.r, 0, 1) * 255.0f + 0.5f);
    UINT32 g = (UINT32)(Clampf(c.g, 0, 1) * 255.0f + 0.5f);
    UINT32 b = (UINT32)(Clampf(c.b, 0, 1) * 255.0f + 0.5f);
    UINT32 a = (UINT32)(Clampf(c.a, 0, 1) * 255.0f + 0.5f);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

constexpr double kPi = 3.14159265358979323846;