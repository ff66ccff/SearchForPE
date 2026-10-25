#include "search.h"

SearchResult SearchEngine::Execute(const std::vector<Question>& questions, const SearchFilter& filter) {
    auto startTime = std::chrono::high_resolution_clock::now();
    SearchResult result;

    // Parse keywords
    std::wstring queryLower = ToLower(filter.query);
    std::wstringstream ss(queryLower);
    std::wstring word;
    while (ss >> word) {
        if (!word.empty()) {
            result.keywords.push_back(word);
        }
    }

    result.items.reserve(questions.size());

    bool filterJudgeOnly = (filter.typeFilter == L"判断题");
    bool filterChoiceOnly = (filter.typeFilter == L"选择题");
    bool hasKeywords = !result.keywords.empty();

    for (const auto& q : questions) {
        // Type filter check
        if (filterJudgeOnly) {
            if (q.type != L"判断题") continue;
        } else if (filterChoiceOnly) {
            if (q.type == L"判断题") continue;
        }

        // Keywords check
        if (!hasKeywords) {
            result.items.push_back(&q);
            continue;
        }

        bool matchAll = true;
        for (const auto& kw : result.keywords) {
            bool kwFound = (q.issue_lower.find(kw) != std::wstring::npos);
            if (!kwFound && filter.searchInOptions) {
                kwFound = (q.options_lower_combined.find(kw) != std::wstring::npos);
            }
            if (!kwFound) {
                matchAll = false;
                break;
            }
        }

        if (matchAll) {
            result.items.push_back(&q);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    return result;
}
