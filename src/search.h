#pragma once
#include "common.h"

struct SearchFilter {
    std::wstring query;
    std::wstring typeFilter = L"全部"; // L"全部", L"判断题", L"选择题"
    bool searchInOptions = true;
};

struct SearchResult {
    std::vector<const Question*> items;
    std::vector<std::wstring> keywords; // split query keywords for highlighting
    double elapsedMs = 0.0;
};

class SearchEngine {
public:
    static SearchResult Execute(const std::vector<Question>& questions, const SearchFilter& filter);
};
