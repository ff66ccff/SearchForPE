#include "common.h"
#include "theme.h"
#include "database.h"
#include "search.h"
#include "ui_card.h"
#include "resource.h"

#pragma comment(lib, "gdiplus")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "uxtheme")

// Window Dimensions & Layout Constants (Base at 96 DPI)
const int BASE_WINDOW_W = 1040;
const int BASE_WINDOW_H = 780;
const int BASE_HEADER_H = 72;
const int BASE_CONTROLS_H = 92;
const int BASE_STATUS_H = 34;
const int BASE_PAGINATION_H = 48;

// IDs for controls and timers
const int ID_SEARCH_EDIT   = 1001;
const int ID_CLEAR_BTN     = 1002;
const int ID_SEARCH_BTN    = 1003;
const int ID_TIMER_DEBOUNCE = 2001;
const int ID_TIMER_FEEDBACK = 2002;

class MainWindow {
public:
    MainWindow() 
        : m_theme(GetLightTheme())
        , m_dpi(96)
        , m_currentPage(0)
        , m_pageSize(20)
        , m_scrollY(0)
        , m_maxScrollY(0)
        , m_searchInOptions(true)
        , m_typeFilter(L"全部")
    {}

    bool Create(HINSTANCE hInstance, int nCmdShow);
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void UpdateDpi(UINT dpi);
    void UpdateTheme(bool dark);
    void LayoutControls();
    void RunSearch(bool resetPage = true);
    void UpdatePageCards();
    void SetPage(int page);
    void ScrollTo(int newY);

    void OnPaint(HDC hdc);
    void OnLButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(short delta);

private:
    HWND m_hwnd = NULL;
    HWND m_hwndEdit = NULL;
    HFONT m_hFontEdit = NULL;
    HBRUSH m_hBrControls = NULL;
    HBRUSH m_hBrEdit = NULL;

    UINT m_dpi;
    ColorScheme m_theme;
    Database m_db;
    SearchResult m_searchResult;

    // Filter & Paging state
    std::wstring m_typeFilter;
    bool m_searchInOptions;
    int m_currentPage;
    int m_pageSize;
    int m_totalPages = 1;

    // Scrolling & Layout
    float m_scrollY;
    float m_maxScrollY;
    std::vector<CardLayout> m_cardLayouts;

    // Interactive button rects in Header/Controls/Pagination
    Gdiplus::RectF m_rcThemeBtn;
    Gdiplus::RectF m_rcFilterAll;
    Gdiplus::RectF m_rcFilterJudge;
    Gdiplus::RectF m_rcFilterChoice;
    Gdiplus::RectF m_rcOptionToggle;
    Gdiplus::RectF m_rcPageSize20;
    Gdiplus::RectF m_rcPageSize50;
    Gdiplus::RectF m_rcPageSize100;
    
    Gdiplus::RectF m_rcPagePrev;
    Gdiplus::RectF m_rcPageNext;
    Gdiplus::RectF m_rcPageFirst;
    Gdiplus::RectF m_rcPageLast;

    // Hover states
    bool m_hoverThemeBtn = false;
    bool m_hoverFilterAll = false;
    bool m_hoverFilterJudge = false;
    bool m_hoverFilterChoice = false;
    bool m_hoverOptionToggle = false;
    bool m_hoverPagePrev = false;
    bool m_hoverPageNext = false;
    bool m_hoverPageFirst = false;
    bool m_hoverPageLast = false;

    std::wstring m_statusMsg = L"就绪";
};

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto setDpi = (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpi) {
            setDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"SportExamSearchMainWnd";
    RegisterClassExW(&wc);

    HDC screen = GetDC(NULL);
    m_dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);

    int winW = Scale(BASE_WINDOW_W, m_dpi);
    int winH = Scale(BASE_WINDOW_H, m_dpi);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (std::max)(0, (screenW - winW) / 2);
    int posY = (std::max)(0, (screenH - winH) / 2);

    m_hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"体育理论题库查询系统 - 极速便携版",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, this
    );

    if (!m_hwnd) return false;

    // Load Database
    std::wstring dbMsg;
    if (!m_db.Load(dbMsg)) {
        MessageBoxW(m_hwnd, dbMsg.c_str(), L"题库加载提示", MB_ICONWARNING);
    }

    UpdateTheme(false);
    RunSearch(true);

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    if (m_hwndEdit) {
        SetFocus(m_hwndEdit);
    }

    return true;
}

void MainWindow::UpdateDpi(UINT dpi) {
    m_dpi = dpi;
    if (m_hFontEdit) {
        DeleteObject(m_hFontEdit);
    }
    m_hFontEdit = CreateFontW(
        -Scale(15, m_dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
    );
    if (m_hwndEdit) {
        SendMessageW(m_hwndEdit, WM_SETFONT, (WPARAM)m_hFontEdit, TRUE);
    }
    LayoutControls();
    UpdatePageCards();
}

void MainWindow::UpdateTheme(bool dark) {
    m_theme = dark ? GetDarkTheme() : GetLightTheme();

    BOOL darkModeAttr = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkModeAttr, sizeof(darkModeAttr));

    if (m_hBrControls) DeleteObject(m_hBrControls);
    if (m_hBrEdit) DeleteObject(m_hBrEdit);
    m_hBrControls = CreateSolidBrush(m_theme.crBgControls);
    m_hBrEdit = CreateSolidBrush(m_theme.crSearchBg);

    if (m_hwndEdit) {
        InvalidateRect(m_hwndEdit, NULL, TRUE);
    }
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::LayoutControls() {
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int clientW = rc.right - rc.left;
    int clientH = rc.bottom - rc.top;

    float headerH = (float)Scale(BASE_HEADER_H, m_dpi);
    float controlsH = (float)Scale(BASE_CONTROLS_H, m_dpi);
    float paginationH = (float)Scale(BASE_PAGINATION_H, m_dpi);

    // Theme Button in Header (top right)
    float themeBtnW = (float)Scale(96, m_dpi);
    float themeBtnH = (float)Scale(32, m_dpi);
    m_rcThemeBtn = Gdiplus::RectF(
        (float)clientW - themeBtnW - (float)Scale(24, m_dpi),
        (headerH - themeBtnH) / 2.0f,
        themeBtnW, themeBtnH
    );

    // Search Edit in Controls row 1
    int editX = Scale(24, m_dpi);
    int editY = (int)headerH + Scale(12, m_dpi);
    int btnSearchW = Scale(70, m_dpi);
    int btnClearW = Scale(42, m_dpi);
    int editW = clientW - editX - btnSearchW - btnClearW - Scale(24 + 16, m_dpi);
    int editH = Scale(38, m_dpi);

    if (!m_hwndEdit) {
        m_hwndEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            editX, editY, editW, editH,
            m_hwnd, (HMENU)(INT_PTR)ID_SEARCH_EDIT,
            GetModuleHandle(NULL), NULL
        );
        SendMessageW(m_hwndEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"输入关键词实时检索题目（空格支持多词组合，即输即显）...");
    } else {
        SetWindowPos(m_hwndEdit, NULL, editX, editY, editW, editH, SWP_NOZORDER);
    }

    // Filter Buttons in Controls row 2
    float row2Y = headerH + (float)Scale(54, m_dpi);
    float filterW = (float)Scale(92, m_dpi);
    float filterH = (float)Scale(28, m_dpi);
    float curX = (float)Scale(24, m_dpi);

    m_rcFilterAll = Gdiplus::RectF(curX, row2Y, filterW, filterH);
    curX += filterW + (float)Scale(8, m_dpi);
    m_rcFilterJudge = Gdiplus::RectF(curX, row2Y, filterW + (float)Scale(10, m_dpi), filterH);
    curX += filterW + (float)Scale(18, m_dpi);
    m_rcFilterChoice = Gdiplus::RectF(curX, row2Y, filterW + (float)Scale(10, m_dpi), filterH);
    curX += filterW + (float)Scale(24, m_dpi);

    // Search in options checkbox/toggle
    float optToggleW = (float)Scale(150, m_dpi);
    m_rcOptionToggle = Gdiplus::RectF(curX, row2Y, optToggleW, filterH);
    curX += optToggleW + (float)Scale(24, m_dpi);

    // Page size buttons on the right of row 2
    float psBtnW = (float)Scale(46, m_dpi);
    float psRight = (float)clientW - (float)Scale(24, m_dpi);
    m_rcPageSize100 = Gdiplus::RectF(psRight - psBtnW, row2Y, psBtnW, filterH);
    m_rcPageSize50 = Gdiplus::RectF(psRight - (psBtnW * 2.0f) - (float)Scale(4, m_dpi), row2Y, psBtnW, filterH);
    m_rcPageSize20 = Gdiplus::RectF(psRight - (psBtnW * 3.0f) - (float)Scale(8, m_dpi), row2Y, psBtnW, filterH);

    // Pagination buttons at bottom
    float pagY = (float)clientH - paginationH + (float)Scale(8, m_dpi);
    float pagBtnW = (float)Scale(74, m_dpi);
    float pagBtnH = (float)Scale(32, m_dpi);
    float pagRight = (float)clientW - (float)Scale(24, m_dpi);

    m_rcPageLast = Gdiplus::RectF(pagRight - (float)Scale(54, m_dpi), pagY, (float)Scale(54, m_dpi), pagBtnH);
    m_rcPageNext = Gdiplus::RectF(m_rcPageLast.X - pagBtnW - (float)Scale(6, m_dpi), pagY, pagBtnW, pagBtnH);
    m_rcPagePrev = Gdiplus::RectF(m_rcPageNext.X - pagBtnW - (float)Scale(110, m_dpi), pagY, pagBtnW, pagBtnH);
    m_rcPageFirst = Gdiplus::RectF(m_rcPagePrev.X - (float)Scale(54, m_dpi) - (float)Scale(6, m_dpi), pagY, (float)Scale(54, m_dpi), pagBtnH);
}

void MainWindow::RunSearch(bool resetPage) {
    wchar_t buf[512] = {0};
    if (m_hwndEdit) {
        GetWindowTextW(m_hwndEdit, buf, 512);
    }

    SearchFilter filter;
    filter.query = buf;
    filter.typeFilter = m_typeFilter;
    filter.searchInOptions = m_searchInOptions;

    m_searchResult = SearchEngine::Execute(m_db.GetQuestions(), filter);

    m_totalPages = (std::max)(1, ((int)m_searchResult.items.size() + m_pageSize - 1) / m_pageSize);
    if (resetPage) {
        m_currentPage = 0;
        m_scrollY = 0;
    } else {
        if (m_currentPage >= m_totalPages) m_currentPage = m_totalPages - 1;
    }

    size_t start = m_currentPage * m_pageSize;
    size_t end = (std::min)(start + m_pageSize, m_searchResult.items.size());

    wchar_t statusBuf[256];
    if (m_searchResult.items.empty()) {
        swprintf(statusBuf, 256, L"未找到相关题目 (用时 %.2f ms)", m_searchResult.elapsedMs);
    } else {
        swprintf(statusBuf, 256, L"共找到 %zu 道题目 (用时 %.2f ms) · 当前显示第 %zu - %zu 题", 
                 m_searchResult.items.size(), m_searchResult.elapsedMs, start + 1, end);
    }
    m_statusMsg = statusBuf;

    UpdatePageCards();
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::UpdatePageCards() {
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int clientW = rc.right - rc.left;
    int clientH = rc.bottom - rc.top;

    float cardX = (float)Scale(24, m_dpi);
    float cardW = (float)clientW - (cardX * 2.0f);
    float gap = (float)Scale(14, m_dpi);

    size_t start = m_currentPage * m_pageSize;
    size_t count = (std::min)((size_t)m_pageSize, m_searchResult.items.size() - (std::min)(m_searchResult.items.size(), start));

    HDC hdc = GetDC(m_hwnd);
    {
        Gdiplus::Graphics g(hdc);
        float endY = CardRenderer::MeasurePageCards(
            g, m_searchResult.items, start, count,
            cardX, cardW, 0.0f, gap, m_dpi, m_cardLayouts
        );

        float headerTotalH = (float)Scale(BASE_HEADER_H + BASE_CONTROLS_H + BASE_STATUS_H, m_dpi);
        float pagH = (float)Scale(BASE_PAGINATION_H, m_dpi);
        float visibleH = (float)clientH - headerTotalH - pagH;

        m_maxScrollY = (std::max)(0.0f, endY - visibleH);
        if (m_scrollY > m_maxScrollY) m_scrollY = m_maxScrollY;
    }
    ReleaseDC(m_hwnd, hdc);
}

void MainWindow::SetPage(int page) {
    if (page < 0) page = 0;
    if (page >= m_totalPages) page = m_totalPages - 1;
    if (m_currentPage != page) {
        m_currentPage = page;
        m_scrollY = 0;
        
        size_t start = m_currentPage * m_pageSize;
        size_t end = (std::min)(start + m_pageSize, m_searchResult.items.size());
        wchar_t statusBuf[256];
        swprintf(statusBuf, 256, L"共找到 %zu 道题目 (用时 %.2f ms) · 当前显示第 %zu - %zu 题", 
                 m_searchResult.items.size(), m_searchResult.elapsedMs, start + 1, end);
        m_statusMsg = statusBuf;

        UpdatePageCards();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::ScrollTo(int newY) {
    float clamped = (std::max)(0.0f, (std::min)((float)newY, m_maxScrollY));
    if (m_scrollY != clamped) {
        m_scrollY = clamped;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::OnPaint(HDC hdc) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

    {
        Gdiplus::Graphics g(memDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

        // 1. Fill entire window background
        Gdiplus::SolidBrush brWin(m_theme.bgWindow);
        g.FillRectangle(&brWin, 0, 0, width, height);

        float headerH = (float)Scale(BASE_HEADER_H, m_dpi);
        float controlsH = (float)Scale(BASE_CONTROLS_H, m_dpi);
        float statusH = (float)Scale(BASE_STATUS_H, m_dpi);
        float pagH = (float)Scale(BASE_PAGINATION_H, m_dpi);

        // 2. Draw Header
        Gdiplus::SolidBrush brHeader(m_theme.bgHeader);
        g.FillRectangle(&brHeader, 0.0f, 0.0f, (float)width, headerH);
        Gdiplus::Pen penBorder(m_theme.borderCard, 1.0f);
        g.DrawLine(&penBorder, 0.0f, headerH, (float)width, headerH);

        // App Logo Icon Badge
        float logoX = (float)Scale(24, m_dpi);
        float logoY = (float)Scale(18, m_dpi);
        float logoSize = (float)Scale(34, m_dpi);
        Gdiplus::RectF rcLogo(logoX, logoY, logoSize, logoSize);
        Gdiplus::SolidBrush brLogoBg(m_theme.accentBlue);
        CardRenderer::DrawRoundedRect(g, &brLogoBg, nullptr, rcLogo, (float)Scale(8, m_dpi));

        Gdiplus::Font fontLogo(L"Microsoft YaHei UI", (float)Scale(12, m_dpi), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brWhite(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::StringFormat sfCenter;
        sfCenter.SetAlignment(Gdiplus::StringAlignmentCenter);
        sfCenter.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(L"题", -1, &fontLogo, rcLogo, &sfCenter, &brWhite);

        // Header Title & Subtitle
        float titleX = logoX + logoSize + (float)Scale(12, m_dpi);
        Gdiplus::Font fontTitle(L"Microsoft YaHei UI", (float)Scale(17, m_dpi), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brTitle(m_theme.textPrimary);
        g.DrawString(L"体育理论题库查询系统", -1, &fontTitle, Gdiplus::PointF(titleX, (float)Scale(14, m_dpi)), &brTitle);

        wchar_t subBuf[256];
        swprintf(subBuf, 256, L"海量 %d 道题库内置 · 毫秒级即时检索 · 判断 / 单选 / 多选全收录", m_db.GetTotalCount());
        Gdiplus::Font fontSub(L"Microsoft YaHei UI", (float)Scale(10.5f, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brSub(m_theme.textSecondary);
        g.DrawString(subBuf, -1, &fontSub, Gdiplus::PointF(titleX, (float)Scale(42, m_dpi)), &brSub);

        // Header Theme Button
        Gdiplus::SolidBrush brThemeBtn(m_hoverThemeBtn ? m_theme.btnActionHoverBg : m_theme.btnActionBg);
        Gdiplus::Pen penThemeBtn(m_hoverThemeBtn ? m_theme.borderCardHover : m_theme.btnActionBorder, 1.0f);
        CardRenderer::DrawRoundedRect(g, &brThemeBtn, &penThemeBtn, m_rcThemeBtn, (float)Scale(16, m_dpi));
        
        Gdiplus::Font fontThemeBtn(L"Microsoft YaHei UI", (float)Scale(11, m_dpi), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brThemeText(m_theme.textPrimary);
        g.DrawString(m_theme.isDark ? L"浅色模式" : L"深色模式", -1, &fontThemeBtn, m_rcThemeBtn, &sfCenter, &brThemeText);

        // 3. Draw Controls Bar
        Gdiplus::SolidBrush brControls(m_theme.bgControls);
        g.FillRectangle(&brControls, 0.0f, headerH, (float)width, controlsH);
        g.DrawLine(&penBorder, 0.0f, headerH + controlsH, (float)width, headerH + controlsH);

        // Search Action Buttons next to Search Edit
        float btnSearchW = (float)Scale(70, m_dpi);
        float btnClearW = (float)Scale(42, m_dpi);
        float btnY = headerH + (float)Scale(12, m_dpi);
        float btnH = (float)Scale(38, m_dpi);
        float btnSearchX = (float)width - (float)Scale(24, m_dpi) - btnSearchW;
        float btnClearX = btnSearchX - btnClearW - (float)Scale(8, m_dpi);

        // Clear button (✕)
        Gdiplus::RectF rcClear(btnClearX, btnY, btnClearW, btnH);
        Gdiplus::SolidBrush brClearBg(m_theme.btnActionBg);
        Gdiplus::Pen penClear(m_theme.borderInput, 1.0f);
        CardRenderer::DrawRoundedRect(g, &brClearBg, &penClear, rcClear, (float)Scale(6, m_dpi));
        Gdiplus::Font fontClear(L"Microsoft YaHei UI", (float)Scale(13, m_dpi), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brClearText(m_theme.textSecondary);
        g.DrawString(L"✕", -1, &fontClear, rcClear, &sfCenter, &brClearText);

        // Search button
        Gdiplus::RectF rcSearch(btnSearchX, btnY, btnSearchW, btnH);
        Gdiplus::SolidBrush brSearchBtn(m_theme.accentBlue);
        CardRenderer::DrawRoundedRect(g, &brSearchBtn, nullptr, rcSearch, (float)Scale(6, m_dpi));
        Gdiplus::Font fontSearchBtn(L"Microsoft YaHei UI", (float)Scale(12, m_dpi), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        g.DrawString(L"搜索", -1, &fontSearchBtn, rcSearch, &sfCenter, &brWhite);

        // Type Filter Pills
        auto DrawFilterPill = [&](const Gdiplus::RectF& rc, const wchar_t* text, bool selected, bool hovered) {
            Gdiplus::SolidBrush bg(selected ? m_theme.accentBlue : (hovered ? m_theme.btnActionHoverBg : m_theme.bgCard));
            Gdiplus::Pen pen(selected ? m_theme.accentBlue : (hovered ? m_theme.borderCardHover : m_theme.borderCard), 1.0f);
            Gdiplus::SolidBrush txt(selected ? Gdiplus::Color(255, 255, 255, 255) : m_theme.textPrimary);
            Gdiplus::Font fontPill(L"Microsoft YaHei UI", (float)Scale(11, m_dpi), selected ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            CardRenderer::DrawRoundedRect(g, &bg, &pen, rc, (float)Scale(6, m_dpi));
            g.DrawString(text, -1, &fontPill, rc, &sfCenter, &txt);
        };

        wchar_t fAllStr[64], fJudgeStr[64], fChoiceStr[64];
        swprintf(fAllStr, 64, L"全部 (%d)", m_db.GetTotalCount());
        swprintf(fJudgeStr, 64, L"判断题 (%d)", m_db.GetJudgeCount());
        swprintf(fChoiceStr, 64, L"选择题 (%d)", m_db.GetChoiceCount());

        DrawFilterPill(m_rcFilterAll, fAllStr, m_typeFilter == L"全部", m_hoverFilterAll);
        DrawFilterPill(m_rcFilterJudge, fJudgeStr, m_typeFilter == L"判断题", m_hoverFilterJudge);
        DrawFilterPill(m_rcFilterChoice, fChoiceStr, m_typeFilter == L"选择题", m_hoverFilterChoice);

        // Option search checkbox toggle
        Gdiplus::SolidBrush brOptTxt(m_theme.textPrimary);
        Gdiplus::Font fontOptToggle(L"Microsoft YaHei UI", (float)Scale(11, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        std::wstring optToggleStr = m_searchInOptions ? L"☑ 同时搜索选项内容" : L"☐ 同时搜索选项内容";
        Gdiplus::StringFormat sfLeftVCenter;
        sfLeftVCenter.SetAlignment(Gdiplus::StringAlignmentNear);
        sfLeftVCenter.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(optToggleStr.c_str(), -1, &fontOptToggle, m_rcOptionToggle, &sfLeftVCenter, &brOptTxt);

        // Page Size Buttons
        auto DrawPageSizeBtn = [&](const Gdiplus::RectF& rc, const wchar_t* text, bool selected) {
            Gdiplus::SolidBrush bg(selected ? m_theme.accentBlue : m_theme.bgCard);
            Gdiplus::Pen pen(selected ? m_theme.accentBlue : m_theme.borderCard, 1.0f);
            Gdiplus::SolidBrush txt(selected ? Gdiplus::Color(255, 255, 255, 255) : m_theme.textSecondary);
            Gdiplus::Font fontPs(L"Microsoft YaHei UI", (float)Scale(10, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            CardRenderer::DrawRoundedRect(g, &bg, &pen, rc, (float)Scale(4, m_dpi));
            g.DrawString(text, -1, &fontPs, rc, &sfCenter, &txt);
        };
        DrawPageSizeBtn(m_rcPageSize20, L"20条", m_pageSize == 20);
        DrawPageSizeBtn(m_rcPageSize50, L"50条", m_pageSize == 50);
        DrawPageSizeBtn(m_rcPageSize100, L"100条", m_pageSize == 100);

        // 4. Status Bar
        float statusY = headerH + controlsH;
        Gdiplus::SolidBrush brStatus(m_theme.bgWindow);
        g.FillRectangle(&brStatus, 0.0f, statusY, (float)width, statusH);
        
        Gdiplus::Font fontStatus(L"Microsoft YaHei UI", (float)Scale(11, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brStatusTxt(m_theme.textSecondary);
        Gdiplus::RectF rcStatus((float)Scale(26, m_dpi), statusY, (float)width - (float)Scale(52, m_dpi), statusH);
        g.DrawString(m_statusMsg.c_str(), -1, &fontStatus, rcStatus, &sfLeftVCenter, &brStatusTxt);

        // 5. Cards Area (Clipped with scroll translation)
        float cardAreaY = statusY + statusH;
        float cardAreaH = (float)height - cardAreaY - pagH;

        if (cardAreaH > 0) {
            g.SetClip(Gdiplus::RectF(0.0f, cardAreaY, (float)width, cardAreaH));

            size_t start = m_currentPage * m_pageSize;
            size_t count = m_cardLayouts.size();

            if (count == 0) {
                Gdiplus::Font fontEmpty(L"Microsoft YaHei UI", (float)Scale(14, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                Gdiplus::SolidBrush brEmpty(m_theme.textMuted);
                Gdiplus::RectF rcEmpty(0.0f, cardAreaY + (float)Scale(60, m_dpi), (float)width, (float)Scale(80, m_dpi));
                g.DrawString(L"没有找到相关题目\n\n试试其他关键词或修改筛选条件吧", -1, &fontEmpty, rcEmpty, &sfCenter, &brEmpty);
            } else {
                for (size_t i = 0; i < count; ++i) {
                    size_t itemIdx = start + i;
                    if (itemIdx >= m_searchResult.items.size()) break;

                    CardLayout cardLayout = m_cardLayouts[i];
                    cardLayout.rcCard.Y += cardAreaY - m_scrollY;
                    cardLayout.rcCopyQuestion.Y += cardAreaY - m_scrollY;
                    cardLayout.rcCopyAll.Y += cardAreaY - m_scrollY;

                    if (cardLayout.rcCard.Y + cardLayout.rcCard.Height < cardAreaY) continue;
                    if (cardLayout.rcCard.Y > cardAreaY + cardAreaH) break;

                    CardRenderer::RenderCard(
                        g,
                        *m_searchResult.items[itemIdx],
                        itemIdx + 1,
                        cardLayout,
                        m_searchResult.keywords,
                        m_theme,
                        m_dpi
                    );
                }
            }

            g.ResetClip();
        }

        // 6. Pagination Bar (Bottom)
        float pagAreaY = (float)height - pagH;
        Gdiplus::SolidBrush brPag(m_theme.bgHeader);
        g.FillRectangle(&brPag, 0.0f, pagAreaY, (float)width, pagH);
        g.DrawLine(&penBorder, 0.0f, pagAreaY, (float)width, pagAreaY);

        Gdiplus::Font fontHint(L"Microsoft YaHei UI", (float)Scale(10, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brHint(m_theme.textMuted);
        Gdiplus::RectF rcHint((float)Scale(24, m_dpi), pagAreaY, (float)Scale(420, m_dpi), pagH);
        g.DrawString(L"快捷键：[Ctrl+F] 搜索  [PgUp/PgDn] 翻页  [Ctrl+T] 主题  [Esc] 清除", -1, &fontHint, rcHint, &sfLeftVCenter, &brHint);

        auto DrawPagBtn = [&](const Gdiplus::RectF& rc, const wchar_t* text, bool disabled, bool hovered) {
            Gdiplus::SolidBrush bg(disabled ? m_theme.bgControls : (hovered ? m_theme.btnActionHoverBg : m_theme.btnActionBg));
            Gdiplus::Pen pen(disabled ? m_theme.borderCard : (hovered ? m_theme.borderCardHover : m_theme.btnActionBorder), 1.0f);
            Gdiplus::SolidBrush txt(disabled ? m_theme.textMuted : m_theme.textPrimary);
            Gdiplus::Font fontPag(L"Microsoft YaHei UI", (float)Scale(11, m_dpi), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            CardRenderer::DrawRoundedRect(g, &bg, &pen, rc, (float)Scale(6, m_dpi));
            g.DrawString(text, -1, &fontPag, rc, &sfCenter, &txt);
        };

        DrawPagBtn(m_rcPageFirst, L"首页", m_currentPage == 0, m_hoverPageFirst);
        DrawPagBtn(m_rcPagePrev, L"◀ 上一页", m_currentPage == 0, m_hoverPagePrev);

        wchar_t pageStr[64];
        swprintf(pageStr, 64, L"第 %d / %d 页", m_currentPage + 1, m_totalPages);
        Gdiplus::RectF rcPageInfo(m_rcPagePrev.X + m_rcPagePrev.Width, pagAreaY, m_rcPageNext.X - (m_rcPagePrev.X + m_rcPagePrev.Width), pagH);
        Gdiplus::Font fontPageInfo(L"Microsoft YaHei UI", (float)Scale(11, m_dpi), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brPageInfo(m_theme.textPrimary);
        g.DrawString(pageStr, -1, &fontPageInfo, rcPageInfo, &sfCenter, &brPageInfo);

        DrawPagBtn(m_rcPageNext, L"下一页 ▶", m_currentPage >= m_totalPages - 1, m_hoverPageNext);
        DrawPagBtn(m_rcPageLast, L"尾页", m_currentPage >= m_totalPages - 1, m_hoverPageLast);
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

void MainWindow::OnLButtonDown(int x, int y) {
    Gdiplus::PointF pt((float)x, (float)y);

    if (m_rcThemeBtn.Contains(pt)) {
        UpdateTheme(!m_theme.isDark);
        return;
    }

    if (m_rcFilterAll.Contains(pt)) {
        m_typeFilter = L"全部";
        RunSearch(true);
        return;
    }
    if (m_rcFilterJudge.Contains(pt)) {
        m_typeFilter = L"判断题";
        RunSearch(true);
        return;
    }
    if (m_rcFilterChoice.Contains(pt)) {
        m_typeFilter = L"选择题";
        RunSearch(true);
        return;
    }

    if (m_rcOptionToggle.Contains(pt)) {
        m_searchInOptions = !m_searchInOptions;
        RunSearch(true);
        return;
    }

    if (m_rcPageSize20.Contains(pt) && m_pageSize != 20) {
        m_pageSize = 20;
        RunSearch(true);
        return;
    }
    if (m_rcPageSize50.Contains(pt) && m_pageSize != 50) {
        m_pageSize = 50;
        RunSearch(true);
        return;
    }
    if (m_rcPageSize100.Contains(pt) && m_pageSize != 100) {
        m_pageSize = 100;
        RunSearch(true);
        return;
    }

    // Clear Button & Search Button
    float headerH = (float)Scale(BASE_HEADER_H, m_dpi);
    float btnSearchW = (float)Scale(70, m_dpi);
    float btnClearW = (float)Scale(42, m_dpi);
    float btnY = headerH + (float)Scale(12, m_dpi);
    float btnH = (float)Scale(38, m_dpi);
    RECT rcCl;
    GetClientRect(m_hwnd, &rcCl);
    float btnSearchX = (float)(rcCl.right - rcCl.left) - (float)Scale(24, m_dpi) - btnSearchW;
    float btnClearX = btnSearchX - btnClearW - (float)Scale(8, m_dpi);

    if (Gdiplus::RectF(btnClearX, btnY, btnClearW, btnH).Contains(pt)) {
        SetWindowTextW(m_hwndEdit, L"");
        SetFocus(m_hwndEdit);
        RunSearch(true);
        return;
    }

    if (Gdiplus::RectF(btnSearchX, btnY, btnSearchW, btnH).Contains(pt)) {
        RunSearch(true);
        return;
    }

    if (m_rcPageFirst.Contains(pt) && m_currentPage > 0) {
        SetPage(0);
        return;
    }
    if (m_rcPagePrev.Contains(pt) && m_currentPage > 0) {
        SetPage(m_currentPage - 1);
        return;
    }
    if (m_rcPageNext.Contains(pt) && m_currentPage < m_totalPages - 1) {
        SetPage(m_currentPage + 1);
        return;
    }
    if (m_rcPageLast.Contains(pt) && m_currentPage < m_totalPages - 1) {
        SetPage(m_totalPages - 1);
        return;
    }

    // Card Copy Buttons Hit Test
    float cardAreaY = (float)Scale(BASE_HEADER_H + BASE_CONTROLS_H + BASE_STATUS_H, m_dpi);
    float pagH = (float)Scale(BASE_PAGINATION_H, m_dpi);
    float cardAreaH = (float)(rcCl.bottom - rcCl.top) - cardAreaY - pagH;

    if (pt.Y >= cardAreaY && pt.Y <= cardAreaY + cardAreaH) {
        size_t start = m_currentPage * m_pageSize;
        for (size_t i = 0; i < m_cardLayouts.size(); ++i) {
            size_t itemIdx = start + i;
            if (itemIdx >= m_searchResult.items.size()) break;

            CardLayout& layout = m_cardLayouts[i];
            Gdiplus::RectF rcQ = layout.rcCopyQuestion;
            rcQ.Y += cardAreaY - m_scrollY;
            Gdiplus::RectF rcAll = layout.rcCopyAll;
            rcAll.Y += cardAreaY - m_scrollY;

            const Question* q = m_searchResult.items[itemIdx];

            if (rcQ.Contains(pt)) {
                CopyToClipboard(m_hwnd, q->issue);
                layout.copiedState = 1;
                layout.copiedTime = std::chrono::steady_clock::now();
                m_statusMsg = L"✓ 已复制题目到剪贴板";
                SetTimer(m_hwnd, ID_TIMER_FEEDBACK, 2000, NULL);
                InvalidateRect(m_hwnd, NULL, FALSE);
                return;
            }

            if (rcAll.Contains(pt)) {
                std::wstring allText = L"【" + q->type + L"】" + q->issue + L"\r\n";
                for (const auto& opt : q->options) {
                    allText += opt + L"\r\n";
                }
                allText += L"正确答案：" + q->answer;
                CopyToClipboard(m_hwnd, allText);
                layout.copiedState = 2;
                layout.copiedTime = std::chrono::steady_clock::now();
                m_statusMsg = L"✓ 已复制全题与答案到剪贴板";
                SetTimer(m_hwnd, ID_TIMER_FEEDBACK, 2000, NULL);
                InvalidateRect(m_hwnd, NULL, FALSE);
                return;
            }
        }
    }
}

void MainWindow::OnMouseMove(int x, int y) {
    Gdiplus::PointF pt((float)x, (float)y);
    bool needRepaint = false;

    auto CheckHover = [&](bool& hoverVar, const Gdiplus::RectF& rc) {
        bool now = rc.Contains(pt);
        if (hoverVar != now) {
            hoverVar = now;
            needRepaint = true;
        }
    };

    CheckHover(m_hoverThemeBtn, m_rcThemeBtn);
    CheckHover(m_hoverFilterAll, m_rcFilterAll);
    CheckHover(m_hoverFilterJudge, m_rcFilterJudge);
    CheckHover(m_hoverFilterChoice, m_rcFilterChoice);
    CheckHover(m_hoverPageFirst, m_rcPageFirst);
    CheckHover(m_hoverPagePrev, m_rcPagePrev);
    CheckHover(m_hoverPageNext, m_rcPageNext);
    CheckHover(m_hoverPageLast, m_rcPageLast);

    float cardAreaY = (float)Scale(BASE_HEADER_H + BASE_CONTROLS_H + BASE_STATUS_H, m_dpi);
    for (size_t i = 0; i < m_cardLayouts.size(); ++i) {
        CardLayout& layout = m_cardLayouts[i];
        Gdiplus::RectF rcCard = layout.rcCard;
        rcCard.Y += cardAreaY - m_scrollY;
        Gdiplus::RectF rcQ = layout.rcCopyQuestion;
        rcQ.Y += cardAreaY - m_scrollY;
        Gdiplus::RectF rcAll = layout.rcCopyAll;
        rcAll.Y += cardAreaY - m_scrollY;

        bool cardHov = rcCard.Contains(pt);
        bool qHov = rcQ.Contains(pt);
        bool allHov = rcAll.Contains(pt);

        if (layout.isHovered != cardHov || layout.isCopyQHovered != qHov || layout.isCopyAllHovered != allHov) {
            layout.isHovered = cardHov;
            layout.isCopyQHovered = qHov;
            layout.isCopyAllHovered = allHov;
            needRepaint = true;
        }
    }

    if (needRepaint) {
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::OnMouseWheel(short delta) {
    int scrollAmount = Scale(60, m_dpi);
    if (delta > 0) {
        ScrollTo((int)m_scrollY - scrollAmount);
    } else {
        ScrollTo((int)m_scrollY + scrollAmount);
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        UpdateDpi(m_dpi);
        return 0;
    }
    case WM_DPICHANGED: {
        UINT newDpi = HIWORD(wParam);
        RECT* prc = (RECT*)lParam;
        SetWindowPos(hwnd, NULL, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, SWP_NOZORDER | SWP_NOACTIVATE);
        UpdateDpi(newDpi);
        return 0;
    }
    case WM_SIZE: {
        LayoutControls();
        UpdatePageCards();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PRINT:
    case WM_PRINTCLIENT:
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = (msg == WM_PAINT) ? BeginPaint(hwnd, &ps) : (HDC)wParam;
        OnPaint(hdc);
        if (msg == WM_PAINT) EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }
    case WM_MOUSEMOVE: {
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }
    case WM_MOUSEWHEEL: {
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_SEARCH_EDIT) {
            if (code == EN_CHANGE) {
                SetTimer(hwnd, ID_TIMER_DEBOUNCE, 50, NULL);
            }
        }
        return 0;
    }
    case WM_TIMER: {
        if (wParam == ID_TIMER_DEBOUNCE) {
            KillTimer(hwnd, ID_TIMER_DEBOUNCE);
            RunSearch(true);
        } else if (wParam == ID_TIMER_FEEDBACK) {
            KillTimer(hwnd, ID_TIMER_FEEDBACK);
            for (auto& l : m_cardLayouts) {
                l.copiedState = 0;
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        if (hwndStatic == m_hwndEdit) {
            SetTextColor(hdcStatic, m_theme.crSearchText);
            SetBkColor(hdcStatic, m_theme.crSearchBg);
            return (LRESULT)m_hBrEdit;
        }
        break;
    }
    case WM_KEYDOWN: {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (ctrl && wParam == 'F') {
            if (m_hwndEdit) {
                SetFocus(m_hwndEdit);
                SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
            }
            return 0;
        } else if (ctrl && wParam == 'T') {
            UpdateTheme(!m_theme.isDark);
            return 0;
        } else if (wParam == VK_ESCAPE) {
            if (m_hwndEdit) {
                SetWindowTextW(m_hwndEdit, L"");
            }
            RunSearch(true);
            return 0;
        } else if (wParam == VK_PRIOR) {
            SetPage(m_currentPage - 1);
            return 0;
        } else if (wParam == VK_NEXT) {
            SetPage(m_currentPage + 1);
            return 0;
        }
        break;
    }
    case WM_DESTROY: {
        if (m_hFontEdit) DeleteObject(m_hFontEdit);
        if (m_hBrControls) DeleteObject(m_hBrControls);
        if (m_hBrEdit) DeleteObject(m_hBrEdit);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int MainWindow::Run() {
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (ctrl && msg.wParam == 'F') {
                if (m_hwndEdit) {
                    SetFocus(m_hwndEdit);
                    SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
                }
                continue;
            } else if (ctrl && msg.wParam == 'T') {
                UpdateTheme(!m_theme.isDark);
                continue;
            } else if (msg.wParam == VK_ESCAPE) {
                if (m_hwndEdit) {
                    SetWindowTextW(m_hwndEdit, L"");
                }
                RunSearch(true);
                continue;
            } else if (msg.hwnd != m_hwndEdit) {
                if (msg.wParam == VK_PRIOR) {
                    SetPage(m_currentPage - 1);
                    continue;
                } else if (msg.wParam == VK_NEXT) {
                    SetPage(m_currentPage + 1);
                    continue;
                }
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    int result = 0;
    {
        MainWindow app;
        if (app.Create(hInstance, nCmdShow)) {
            result = app.Run();
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return result;
}
