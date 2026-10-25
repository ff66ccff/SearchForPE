#include "database.h"
#include <fstream>

extern "C" {
    typedef struct sqlite3 sqlite3;
    typedef struct sqlite3_stmt sqlite3_stmt;
    int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs);
    int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
    int sqlite3_step(sqlite3_stmt*);
    const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
    int sqlite3_finalize(sqlite3_stmt *pStmt);
    int sqlite3_close(sqlite3*);
    const char *sqlite3_errmsg(sqlite3*);
}

std::wstring Database::ResolveDatabasePath() {
    // 1. Check current directory
    DWORD attr = GetFileAttributesW(L"question.db");
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return L"question.db";
    }

    // 2. Check next to exe
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        std::wstring adjacentDb = std::wstring(exePath) + L"question.db";
        DWORD adjAttr = GetFileAttributesW(adjacentDb.c_str());
        if (adjAttr != INVALID_FILE_ATTRIBUTES && !(adjAttr & FILE_ATTRIBUTE_DIRECTORY)) {
            return adjacentDb;
        }
    }

    // 3. Check embedded resource (IDR_QUESTION_DB = 101, RT_RCDATA = 10)
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(101), MAKEINTRESOURCEW(10));
    if (hRes) {
        HGLOBAL hData = LoadResource(NULL, hRes);
        if (hData) {
            DWORD size = SizeofResource(NULL, hRes);
            const void* pData = LockResource(hData);
            if (pData && size > 0) {
                wchar_t tempPath[MAX_PATH] = {0};
                GetTempPathW(MAX_PATH, tempPath);
                std::wstring tempDb = std::wstring(tempPath) + L"SearchForPE_embedded.db";
                // Reuse the previously unpacked copy when it is byte-identical in
                // size: saves ~1.6 MB of disk I/O on every cold start.
                WIN32_FILE_ATTRIBUTE_DATA info = {};
                if (GetFileAttributesExW(tempDb.c_str(), GetFileExInfoStandard, &info) &&
                    info.nFileSizeHigh == 0 && info.nFileSizeLow == size) {
                    return tempDb;
                }
                FILE* fp = _wfopen(tempDb.c_str(), L"wb");
                if (fp) {
                    fwrite(pData, 1, size, fp);
                    fclose(fp);
                    return tempDb;
                }
            }
        }
    }

    return L"";
}

bool Database::Load(std::wstring& outMsg) {
    m_questions.clear();
    m_totalCount = 0;
    m_judgeCount = 0;
    m_choiceCount = 0;

    std::wstring dbPath = ResolveDatabasePath();
    if (dbPath.empty()) {
        outMsg = L"找不到题库文件 question.db 且未发现内嵌资源";
        return false;
    }

    std::string dbPathUtf8 = WideToUtf8(dbPath.c_str());
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(dbPathUtf8.c_str(), &db, 1 /* SQLITE_OPEN_READONLY */, NULL);
    if (rc != 0 || !db) {
        std::string err = db ? sqlite3_errmsg(db) : "无法打开数据库文件";
        outMsg = L"打开数据库失败: " + Utf8ToWide(err.c_str());
        if (db) sqlite3_close(db);
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT ISSUE, CHOICE, ANSWER FROM QUESTIONS;";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != 0 || !stmt) {
        std::string err = sqlite3_errmsg(db);
        outMsg = L"SQL 查询失败: " + Utf8ToWide(err.c_str());
        sqlite3_close(db);
        return false;
    }

    m_questions.reserve(6500);
    int count = 0;

    while (sqlite3_step(stmt) == 100 /* SQLITE_ROW */) {
        Question q;
        q.id = ++count;

        const char* issue_u8 = (const char*)sqlite3_column_text(stmt, 0);
        const char* choice_u8 = (const char*)sqlite3_column_text(stmt, 1);
        const char* answer_u8 = (const char*)sqlite3_column_text(stmt, 2);

        q.issue = Utf8ToWide(issue_u8);
        q.answer = Utf8ToWide(answer_u8);

        if (choice_u8 && strlen(choice_u8) > 0) {
            std::wstring choice_w = Utf8ToWide(choice_u8);
            size_t start = 0;
            while (start < choice_w.size()) {
                size_t end = choice_w.find(L'\n', start);
                if (end == std::wstring::npos) end = choice_w.size();
                std::wstring opt = choice_w.substr(start, end - start);
                size_t p1 = opt.find_first_not_of(L" \r\t");
                size_t p2 = opt.find_last_not_of(L" \r\t");
                if (p1 != std::wstring::npos && p2 != std::wstring::npos) {
                    q.options.push_back(opt.substr(p1, p2 - p1 + 1));
                }
                start = end + 1;
            }
        }

        if (q.options.empty()) {
            q.type = L"判断题";
            m_judgeCount++;
        } else {
            m_choiceCount++;
            if (q.answer.length() > 1) {
                q.type = L"多选题";
            } else {
                q.type = L"单选题";
            }
        }

        q.issue_lower = ToLower(q.issue);
        for (const auto& opt : q.options) {
            q.options_lower_combined += ToLower(opt) + L" ";
        }

        m_questions.push_back(std::move(q));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    m_totalCount = (int)m_questions.size();
    outMsg = L"成功加载 " + std::to_wstring(m_totalCount) + L" 道题目";
    return true;
}