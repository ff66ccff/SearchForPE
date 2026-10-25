#pragma once
#define NOMINMAX
#include <windows.h>
#include <wtypes.h>
#include <propidl.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>
#include <sstream>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

// Data structure for question
struct Question {
    int id = 0;
    std::wstring issue;
    std::vector<std::wstring> options;
    std::wstring answer;
    std::wstring type; // L"判断题", L"单选题", L"多选题"
    std::wstring issue_lower;
    std::wstring options_lower_combined;
};

// String conversion utilities
inline std::wstring Utf8ToWide(const char* utf8, int len = -1) {
    if (!utf8 || len == 0) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, len, NULL, 0);
    if (wlen <= 0) return L"";
    std::wstring result(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8, len, &result[0], wlen);
    if (len == -1 && !result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

inline std::string WideToUtf8(const wchar_t* wide, int len = -1) {
    if (!wide || len == 0) return "";
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide, len, NULL, 0, NULL, NULL);
    if (ulen <= 0) return "";
    std::string result(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide, len, &result[0], ulen, NULL, NULL);
    if (len == -1 && !result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

// Convert string to lower case for case-insensitive search
inline std::wstring ToLower(const std::wstring& str) {
    std::wstring lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return lower;
}

// Clipboard utility
inline bool CopyToClipboard(HWND hwnd, const std::wstring& text) {
    if (!OpenClipboard(hwnd)) return false;
    EmptyClipboard();
    size_t bytes = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hMem) {
        CloseClipboard();
        return false;
    }
    memcpy(GlobalLock(hMem), text.c_str(), bytes);
    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return true;
}

// DPI scale helper
inline int Scale(int val, UINT dpi) {
    return MulDiv(val, (int)dpi, 96);
}
