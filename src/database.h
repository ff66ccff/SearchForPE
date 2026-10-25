#pragma once
#include "common.h"

class Database {
public:
    Database() = default;
    ~Database() = default;

    bool Load(std::wstring& outMsg);

    const std::vector<Question>& GetQuestions() const { return m_questions; }
    int GetTotalCount() const { return m_totalCount; }
    int GetJudgeCount() const { return m_judgeCount; }
    int GetChoiceCount() const { return m_choiceCount; }

private:
    std::vector<Question> m_questions;
    int m_totalCount = 0;
    int m_judgeCount = 0;
    int m_choiceCount = 0;
    
    std::wstring ResolveDatabasePath();
};
