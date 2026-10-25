#include "common.h"
#include "theme.h"
#include "fonts.h"
#include "gfx.h"
#include "result_list.h"
#include "database.h"
#include "search.h"
#include "resource.h"
#include <cstdarg>

namespace {

// Layout constants in DIPs, matching design/layout.md
constexpr float kPadX            = 24.0f;
constexpr float kFieldY          = 14.0f;
constexpr float kFieldH          = 36.0f;
constexpr float kFieldRadius     = 10.0f;
constexpr float kFieldTextLeft   = 40.0f;
constexpr float kFieldClearRight = 6.0f;
constexpr float kThemeBtnSize    = 32.0f;
constexpr float kFilterTop       = 64.0f;
constexpr float kFilterInnerTop  = 8.0f;
constexpr float kFilterH         = 30.0f;
constexpr float kFilterPadX      = 13.0f;
constexpr float kFilterGap       = 8.0f;
constexpr float kHairlineY       = 110.0f;
constexpr float kStatusTop       = 110.0f;
constexpr float kStatusH         = 30.0f;
constexpr float kPanelTop        = 140.0f;
constexpr float kPanelBottomGap  = 16.0f;
constexpr float kPanelMaxWidth   = 1000.0f;
constexpr float kPanelPadX       = 20.0f;
constexpr float kFabSize         = 32.0f;
constexpr float kFabMargin       = 24.0f;
constexpr float kSwitchW         = 30.0f;
constexpr float kSwitchH         = 17.0f;
constexpr float kWheelStep       = 112.0f;

constexpr UINT_PTR kTimerAnim    = 1;
constexpr UINT_PTR kTimerSearch  = 2;
constexpr UINT_PTR kTimerPreload = 3;
constexpr int kSearchDebounceMs  = 35;
constexpr int kAnimIntervalMs    = 10;

constexpr int ID_SEARCH_EDIT     = 1001;

}  // namespace

class App {
public:
    bool Create(HINSTANCE hInstance, int nCmdShow);
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);

    void ApplyDpi(UINT dpi);
    void SetTheme(bool dark);
    void ApplyWindowChrome();
    void Layout();
    void RebuildEditFont();
    void UpdateEditColors();
    void FocusSearch();

    void UpdateSearchResults();
    void StartAnimating();
    void StopAnimatingIfIdle();
    bool AnimTick();

    void Paint();
    void PaintSearchField();
    void PaintToolbar();
    void PaintStatusRow();
    void PaintEmptyState();
    void PaintBackToTop();

    void OnMouseMove(int px, int py);
    void OnLButtonDown(int px, int py);
    void OnLButtonUp(int px, int py);
    void OnMouseWheel(short delta);
    void OnKeyDown(WPARAM key, bool ctrl);

    int  HitTestPill(float x, float y) const;
    bool HitTestThemeButton(float x, float y) const;
    bool HitTestClearButton(float x, float y) const;
    bool HitTestSwitch(float x, float y) const;
    bool HitTestField(float x, float y) const;
    bool HitTestBackToTop(float x, float y) const;
    RectF ClearButtonRect() const;

    float DipX(int px) const { return PixelsToDip(px, m_dpi); }
    float DipY(int py) const { return PixelsToDip(py, m_dpi); }

    float DrawTextBlock(Gfx& gfx, const std::wstring& text, TextRole role, float x, float y, Color color);
    void DrawTextCentered(Gfx& gfx, const std::wstring& text, TextRole role, const RectF& box, Color color);
    void DrawTextLeftVCenter(Gfx& gfx, const std::wstring& text, TextRole role, float x, float y, float h,
                             Color color);
    float MeasureText(const std::wstring& text, TextRole role) const { return m_fonts.MeasureWidth(text, role); }
    void DrawEditPlaceholderIfNeeded();

    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_edit = nullptr;
    WNDPROC m_editProc = nullptr;
    HFONT m_editFont = nullptr;
    HBRUSH m_editBrush = nullptr;

    UINT m_dpi = 96;
    float m_dipW = 0.0f;
    float m_dipH = 0.0f;

    Theme m_theme;
    FontManager m_fonts;
    Gfx m_gfx;
    Database m_db;
    ResultList m_list;
    SearchResult m_search;

    std::wstring m_typeFilter = L"全部";
    bool m_searchInOptions = true;

    int m_hoverPill = -1;
    bool m_hoverThemeBtn = false;
    bool m_hoverClear = false;
    bool m_hoverSwitch = false;
    bool m_hoverBackToTop = false;
    bool m_hoverField = false;

    RectF m_fieldRect;
    RectF m_themeBtnRect;
    RectF m_pillRects[3];
    RectF m_switchRect;
    RectF m_fabRect;
    RectF m_panelRect;

    std::chrono::steady_clock::time_point m_lastTick;
};

// ------------------------------------------------------------------ window
bool App::Create(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        typedef BOOL(WINAPI * SetDpiProc)(DPI_AWARENESS_CONTEXT);
        auto setDpi = (SetDpiProc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (setDpi) setDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = App::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"SportExamSearchWnd";
    if (!RegisterClassExW(&wc)) return false;

    m_theme = MakeLightTheme();
    if (!m_fonts.Init()) return false;
    if (!m_gfx.Init()) return false;
    m_list.Init(&m_fonts);

    HDC screen = GetDC(nullptr);
    m_dpi = (UINT)GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);

    // Size the outer window so the *client* area is exactly 1180x840 DIP.
    RECT desired = { 0, 0, DipToPixels(1180.0f, m_dpi), DipToPixels(840.0f, m_dpi) };
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    typedef BOOL(WINAPI * AdjustProc)(LPRECT, DWORD, BOOL, DWORD, UINT);
    bool adjusted = false;
    if (HMODULE u32 = GetModuleHandleW(L"user32.dll")) {
        auto adjust = (AdjustProc)GetProcAddress(u32, "AdjustWindowRectExForDpi");
        if (adjust) adjusted = adjust(&desired, style, FALSE, 0, m_dpi) == TRUE;
    }
    if (!adjusted) AdjustWindowRectEx(&desired, style, FALSE, 0);
    int winW = desired.right - desired.left;
    int winH = desired.bottom - desired.top;

    RECT workArea = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int posX = workArea.left + ((workArea.right - workArea.left) - winW) / 2;
    int posY = workArea.top + ((workArea.bottom - workArea.top) - winH) / 2;

    m_hwnd = CreateWindowExW(0, wc.lpszClassName, L"体育理论题库查询", style, posX, posY,
                             (std::max)(winW, 600), (std::max)(winH, 400), nullptr, nullptr, hInstance, this);
    if (!m_hwnd) return false;

    std::wstring dbMsg;
    if (!m_db.Load(dbMsg)) {
        MessageBoxW(m_hwnd, dbMsg.c_str(), L"题库加载提示", MB_ICONWARNING);
    }

    Layout();
    ApplyWindowChrome();
    UpdateSearchResults();

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    FocusSearch();
    return true;
}

int App::Run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

// ------------------------------------------------------------------ theme / dpi
void App::ApplyDpi(UINT dpi) {
    m_dpi = dpi;
    RebuildEditFont();
    Layout();
}

void App::SetTheme(bool dark) {
    m_theme = dark ? MakeDarkTheme() : MakeLightTheme();
    ApplyWindowChrome();
    UpdateEditColors();
    if (m_edit) InvalidateRect(m_edit, nullptr, TRUE);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void App::ApplyWindowChrome() {
    if (!m_hwnd) return;
    BOOL dark = m_theme.dark ? TRUE : FALSE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    COLORREF caption = m_theme.captionColor;
    COLORREF text = m_theme.captionText;
    DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
    DwmSetWindowAttribute(m_hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));
    COLORREF borderColor = ToColorRef(m_theme.windowBorder);
    DwmSetWindowAttribute(m_hwnd, 34 /* DWMWA_BORDER_COLOR */, &borderColor, sizeof(borderColor));
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void App::UpdateEditColors() {
    if (m_editBrush) DeleteObject(m_editBrush);
    m_editBrush = CreateSolidBrush(m_theme.editBg);
    if (m_edit) InvalidateRect(m_edit, nullptr, TRUE);
}

void App::RebuildEditFont() {
    if (m_editFont) DeleteObject(m_editFont);
    m_editFont = m_fonts.CreateGdiFont(TextRole::Field, m_dpi);
    if (m_edit && m_editFont) {
        SendMessageW(m_edit, WM_SETFONT, (WPARAM)m_editFont, TRUE);
    }
}

// ------------------------------------------------------------------ layout
void App::Layout() {
    if (!m_hwnd) return;
    RECT rc = {};
    GetClientRect(m_hwnd, &rc);
    m_dipW = PixelsToDip(rc.right - rc.left, m_dpi);
    m_dipH = PixelsToDip(rc.bottom - rc.top, m_dpi);

    m_themeBtnRect = RectF(m_dipW - kFabMargin - kThemeBtnSize, 16.0f, kThemeBtnSize, kThemeBtnSize);
    float fieldRight = m_themeBtnRect.x - 12.0f;
    m_fieldRect = RectF(kPadX, kFieldY, (std::max)(160.0f, fieldRight - kPadX), kFieldH);

    // Result panel first: toolbar rows align to its edges.
    float panelW = (std::min)(kPanelMaxWidth, m_dipW - kPadX * 2.0f);
    float panelX = (m_dipW - panelW) * 0.5f;
    float panelH = (std::max)(120.0f, m_dipH - kPanelTop - kPanelBottomGap);
    m_panelRect = RectF(panelX, kPanelTop, panelW, panelH);

    // filter chips, aligned to the panel column
    std::wstring labels[3] = {
        L"全部", L"判断题", L"选择题"
    };
    std::wstring numbers[3] = {
        FormatInt(m_db.GetTotalCount()), FormatInt(m_db.GetJudgeCount()), FormatInt(m_db.GetChoiceCount())
    };
    float px = m_panelRect.x;
    const float filterY = kFilterTop + kFilterInnerTop;
    for (int i = 0; i < 3; ++i) {
        float labelW = m_fonts.MeasureWidth(labels[i], TextRole::FilterStrong);
        float numW = m_fonts.MeasureWidth(numbers[i], TextRole::Meta);
        float w = labelW + 6.0f + numW + kFilterPadX * 2.0f;
        m_pillRects[i] = RectF(px, filterY, w, kFilterH);
        px += w + kFilterGap;
    }

    m_switchRect = RectF(m_panelRect.Right() - kSwitchW, filterY + (kFilterH - kSwitchH) * 0.5f,
                         kSwitchW, kSwitchH);

    m_fabRect = RectF(m_dipW - kFabMargin - kFabSize, m_dipH - kFabMargin - kFabSize, kFabSize, kFabSize);

    m_list.SetPanel(m_panelRect);
    m_list.SetContentWidth(m_panelRect.w - kPanelPadX * 2.0f);

    if (m_edit) {
        int textLen = GetWindowTextLengthW(m_edit);
        float editX = m_fieldRect.x + kFieldTextLeft;
        float editRight = m_fieldRect.Right() - (textLen > 0 ? 44.0f : 14.0f);
        float editH = 24.0f;
        float editY = m_fieldRect.y + (m_fieldRect.h - editH) * 0.5f;
        SetWindowPos(m_edit, nullptr, DipToPixels(editX, m_dpi), DipToPixels(editY, m_dpi),
                     (std::max)(20, DipToPixels(editRight - editX, m_dpi)), DipToPixels(editH, m_dpi),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// ------------------------------------------------------------------ search
void App::UpdateSearchResults() {
    wchar_t buf[512] = {};
    if (m_edit) GetWindowTextW(m_edit, buf, 512);

    SearchFilter filter;
    filter.query = buf;
    filter.typeFilter = m_typeFilter;
    filter.searchInOptions = m_searchInOptions;
    m_search = SearchEngine::Execute(m_db.GetQuestions(), filter);

    m_list.SetResults(m_search.items, m_search.keywords);
    m_list.SetPanel(m_panelRect);
    m_list.SetContentWidth(m_panelRect.w - kPanelPadX * 2.0f);
    m_list.ScrollTo(0.0f, false);
    m_list.EnsureLoaded();
    Layout();
    KillTimer(m_hwnd, kTimerPreload);
    if (!m_list.AllLoaded()) SetTimer(m_hwnd, kTimerPreload, 16, nullptr);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void App::FocusSearch() {
    if (!m_edit) return;
    SetFocus(m_edit);
    SendMessageW(m_edit, EM_SETSEL, -1, -1);
}

// ------------------------------------------------------------------ animation
void App::StartAnimating() {
    if (!m_hwnd) return;
    m_lastTick = std::chrono::steady_clock::now();
    SetTimer(m_hwnd, kTimerAnim, kAnimIntervalMs, nullptr);
}

void App::StopAnimatingIfIdle() {
    if (m_list.Animating() || m_list.CopyFeedbackActive()) return;
    KillTimer(m_hwnd, kTimerAnim);
}

bool App::AnimTick() {
    auto now = std::chrono::steady_clock::now();
    float dt = (float)std::chrono::duration<double, std::milli>(now - m_lastTick).count();
    m_lastTick = now;
    if (dt <= 0.0f || dt > 100.0f) dt = (float)kAnimIntervalMs;

    bool changed = m_list.Tick(dt);
    m_list.TickCopyFeedback(dt);
    if (m_list.CopyFeedbackActive()) changed = true;

    if (changed) InvalidateRect(m_hwnd, nullptr, FALSE);
    StopAnimatingIfIdle();
    return changed;
}

// ------------------------------------------------------------------ text helpers
float App::DrawTextBlock(Gfx& gfx, const std::wstring& text, TextRole role, float x, float y, Color color) {
    IDWriteTextLayout* layout = m_fonts.MakeLayout(text, role, 4096.0f, false);
    if (!layout) return 0.0f;
    DWRITE_TEXT_METRICS m = {};
    layout->GetMetrics(&m);
    gfx.DrawTextLayout(layout, x, y, color);
    layout->Release();
    return m.height;
}

void App::DrawTextCentered(Gfx& gfx, const std::wstring& text, TextRole role, const RectF& box, Color color) {
    IDWriteTextLayout* layout = m_fonts.MakeLayout(text, role, (std::max)(1.0f, box.w), false);
    if (!layout) return;
    layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    DWRITE_TEXT_METRICS m = {};
    layout->GetMetrics(&m);
    gfx.DrawTextLayout(layout, box.x, box.y + (box.h - m.height) * 0.5f, color);
    layout->Release();
}

void App::DrawTextLeftVCenter(Gfx& gfx, const std::wstring& text, TextRole role, float x, float y, float h,
                              Color color) {
    IDWriteTextLayout* layout = m_fonts.MakeLayout(text, role, 4096.0f, false);
    if (!layout) return;
    DWRITE_TEXT_METRICS m = {};
    layout->GetMetrics(&m);
    gfx.DrawTextLayout(layout, x, y + (h - m.height) * 0.5f, color);
    layout->Release();
}

// ------------------------------------------------------------------ paint
void App::Paint() {
    RECT rc = {};
    GetClientRect(m_hwnd, &rc);
    UINT widthPx = (UINT)(rc.right - rc.left);
    UINT heightPx = (UINT)(rc.bottom - rc.top);
    if (widthPx == 0 || heightPx == 0) return;

    if (FAILED(m_gfx.EnsureTarget(m_hwnd, widthPx, heightPx, (float)m_dpi))) return;
    if (FAILED(m_gfx.BeginDraw())) return;

    m_gfx.Clear(m_theme.window);
    PaintSearchField();
    PaintToolbar();
    PaintStatusRow();
    m_list.Paint(m_gfx, m_theme);
    if (m_list.Empty()) PaintEmptyState();
    PaintBackToTop();

    HRESULT hr = m_gfx.EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void App::PaintSearchField() {
    bool focused = (GetFocus() == m_edit);
    m_gfx.FillRound(m_fieldRect, kFieldRadius, m_theme.surfaceSunken);
    Color borderColor = focused ? m_theme.accent : (m_hoverField ? m_theme.borderStrong : m_theme.border);
    m_gfx.StrokeRound(m_fieldRect.Inset(0.5f, 0.5f), kFieldRadius, borderColor, 1.0f);

    RectF iconBox(m_fieldRect.x + 14.0f, m_fieldRect.y + (m_fieldRect.h - 16.0f) * 0.5f, 16.0f, 16.0f);
    m_gfx.DrawIcon(Icon::Search, iconBox, m_theme.textMuted, 1.6f);

    if (m_edit && GetWindowTextLengthW(m_edit) > 0) {
        RectF clearRect = ClearButtonRect();
        if (m_hoverClear) {
            m_gfx.FillRound(clearRect, 12.0f, m_theme.controlHover);
        }
        RectF clearIcon(clearRect.x + 5.0f, clearRect.y + 5.0f, 14.0f, 14.0f);
        m_gfx.DrawIcon(Icon::Close, clearIcon, m_hoverClear ? m_theme.textSecondary : m_theme.textMuted,
                       1.6f);
    }

    m_gfx.FillRound(m_themeBtnRect, 16.0f,
                    m_hoverThemeBtn ? m_theme.controlHover : WithAlpha(m_theme.controlHover, 0.0f));
    RectF themeIcon(m_themeBtnRect.x + 7.0f, m_themeBtnRect.y + 7.0f, 18.0f, 18.0f);
    m_gfx.DrawIcon(m_theme.dark ? Icon::Sun : Icon::Moon, themeIcon,
                   m_hoverThemeBtn ? m_theme.textPrimary : m_theme.textSecondary, 1.6f);
}

void App::PaintToolbar() {
    std::wstring labels[3] = { L"全部", L"判断题", L"选择题" };
    std::wstring numbers[3] = { FormatInt(m_db.GetTotalCount()), FormatInt(m_db.GetJudgeCount()),
                                FormatInt(m_db.GetChoiceCount()) };
    const std::wstring values[3] = { L"全部", L"判断题", L"选择题" };

    for (int i = 0; i < 3; ++i) {
        bool selected = (m_typeFilter == values[i]);
        bool hovered = (m_hoverPill == i);
        Color bg = selected ? m_theme.chipSelectedBg
                            : (hovered ? m_theme.controlHover : WithAlpha(m_theme.controlHover, 0.0f));
        m_gfx.FillRound(m_pillRects[i], 8.0f, bg);
        if (selected) {
            m_gfx.StrokeRound(m_pillRects[i].Inset(0.5f, 0.5f), 8.0f, m_theme.borderStrong, 1.0f);
        }
        float labelW = MeasureText(labels[i], TextRole::FilterStrong);
        float x = m_pillRects[i].x + kFilterPadX;
        DrawTextLeftVCenter(m_gfx, labels[i], selected ? TextRole::FilterStrong : TextRole::Filter,
                            x, m_pillRects[i].y, kFilterH,
                            selected ? m_theme.textPrimary : m_theme.textSecondary);
        DrawTextLeftVCenter(m_gfx, numbers[i],
                            selected ? TextRole::MetaMedium : TextRole::Meta,
                            x + labelW + 6.0f, m_pillRects[i].y, kFilterH,
                            selected ? m_theme.textPrimary : m_theme.textMuted);
    }

    const std::wstring label = L"检索选项";
    float labelW = MeasureText(label, TextRole::Filter);
    float labelX = m_switchRect.x - 8.0f - labelW;
    DrawTextLeftVCenter(m_gfx, label, TextRole::Filter, labelX,
                        m_switchRect.y - (kFilterH - kSwitchH) * 0.5f, kFilterH, m_theme.textSecondary);

    RectF track = m_switchRect;
    m_gfx.FillRound(track, kSwitchH * 0.5f,
                    m_searchInOptions ? m_theme.switchTrackOn : m_theme.switchTrackOff);
    float knobR = 6.5f;
    float knobCx = m_searchInOptions ? track.Right() - 2.0f - knobR : track.x + 2.0f + knobR;
    m_gfx.FillCircle(knobCx, track.CenterY(), knobR, m_theme.switchThumb);

    m_gfx.FillRect(RectF(0.0f, kHairlineY, m_dipW, 1.0f), m_theme.hairline);
}

void App::PaintStatusRow() {
    std::wstring count = FormatInt((long long)m_list.Count());
    DrawTextLeftVCenter(m_gfx, count, TextRole::StatusStrong, m_panelRect.x, kStatusTop, kStatusH,
                        m_theme.textPrimary);
    float countW = MeasureText(count, TextRole::StatusStrong);
    DrawTextLeftVCenter(m_gfx, L" 条结果", TextRole::Status, m_panelRect.x + countW, kStatusTop, kStatusH,
                        m_theme.textMuted);

    std::wstring latency = FormatMs(m_search.elapsedMs);
    float latencyW = MeasureText(latency, TextRole::Status);
    DrawTextLeftVCenter(m_gfx, latency, TextRole::Status, m_panelRect.Right() - latencyW, kStatusTop,
                        kStatusH, m_theme.textMuted);
}

void App::PaintEmptyState() {
    float cx = m_panelRect.CenterX();
    float cy = m_panelRect.CenterY();
    RectF iconBox(cx - 10.0f, cy - 44.0f, 20.0f, 20.0f);
    m_gfx.DrawIcon(Icon::Search, iconBox, m_theme.textMuted, 1.6f);
    DrawTextCentered(m_gfx, L"没有找到相关题目", TextRole::EmptyTitle,
                     RectF(m_panelRect.x, cy - 10.0f, m_panelRect.w, 20.0f), m_theme.textSecondary);
    DrawTextCentered(m_gfx, L"试试更短的关键词，或切换筛选条件", TextRole::EmptyHint,
                     RectF(m_panelRect.x, cy + 16.0f, m_panelRect.w, 18.0f), m_theme.textMuted);
}

void App::PaintBackToTop() {
    if (m_list.ScrollY() <= 320.0f || m_list.Empty()) return;
    m_gfx.ShadowCircle(m_fabRect.CenterX(), m_fabRect.CenterY() + 1.5f, m_fabRect.w * 0.5f,
                       m_theme.shadowSoft);
    m_gfx.FillCircle(m_fabRect.CenterX(), m_fabRect.CenterY(), m_fabRect.w * 0.5f, m_theme.surface);
    m_gfx.StrokeCircle(m_fabRect.CenterX(), m_fabRect.CenterY(), m_fabRect.w * 0.5f - 0.5f,
                       m_theme.border, 1.0f);
    RectF icon(m_fabRect.x + 8.0f, m_fabRect.y + 8.0f, 16.0f, 16.0f);
    m_gfx.DrawIcon(Icon::ArrowUp, icon, m_hoverBackToTop ? m_theme.textPrimary : m_theme.textSecondary,
                   1.6f);
}

// ------------------------------------------------------------------ hit tests
RectF App::ClearButtonRect() const {
    return RectF(m_fieldRect.Right() - kFieldClearRight - 24.0f, m_fieldRect.y + (m_fieldRect.h - 24.0f) * 0.5f,
                 24.0f, 24.0f);
}

int App::HitTestPill(float x, float y) const {
    for (int i = 0; i < 3; ++i) {
        if (m_pillRects[i].Contains(x, y)) return i;
    }
    return -1;
}

bool App::HitTestThemeButton(float x, float y) const { return m_themeBtnRect.Contains(x, y); }

bool App::HitTestClearButton(float x, float y) const {
    if (!m_edit || GetWindowTextLengthW(m_edit) == 0) return false;
    return ClearButtonRect().Contains(x, y);
}

bool App::HitTestSwitch(float x, float y) const {
    RectF area(m_switchRect.x - 130.0f, m_switchRect.y - 8.0f, 130.0f + m_switchRect.w, kSwitchH + 16.0f);
    return area.Contains(x, y);
}

bool App::HitTestField(float x, float y) const { return m_fieldRect.Contains(x, y); }

bool App::HitTestBackToTop(float x, float y) const {
    return m_list.ScrollY() > 320.0f && !m_list.Empty() && m_fabRect.Contains(x, y);
}

// ------------------------------------------------------------------ input
void App::OnMouseMove(int px, int py) {
    float x = DipX(px);
    float y = DipY(py);
    if (m_list.DraggingScrollbar()) {
        m_list.ScrollbarDragTo(x, y);
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    bool needPaint = false;

    int pill = HitTestPill(x, y);
    bool themeBtn = HitTestThemeButton(x, y);
    bool clearBtn = HitTestClearButton(x, y);
    bool sw = HitTestSwitch(x, y);
    bool field = HitTestField(x, y);
    bool top = HitTestBackToTop(x, y);
    bool scrollbarHover = m_list.HitTestScrollbar(x, y);

    if (pill != m_hoverPill || themeBtn != m_hoverThemeBtn || clearBtn != m_hoverClear ||
        sw != m_hoverSwitch || field != m_hoverField || top != m_hoverBackToTop ||
        scrollbarHover != m_list.ScrollbarHover()) {
        m_hoverPill = pill;
        m_hoverThemeBtn = themeBtn;
        m_hoverClear = clearBtn;
        m_hoverSwitch = sw;
        m_hoverField = field;
        m_hoverBackToTop = top;
        m_list.SetScrollbarHover(scrollbarHover);
        needPaint = true;
    }

    int item = m_list.ItemIndexAt(x, y);
    bool copyHit = item >= 0 && m_list.CopyButtonHit(item, x, y);
    if (item != m_list.HoveredIndex() || copyHit != m_list.HoveredCopy()) {
        m_list.SetHoverIndex(item);
        m_list.SetHoveredCopy(copyHit);
        needPaint = true;
    }

    if (needPaint) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void App::OnLButtonDown(int px, int py) {
    float x = DipX(px);
    float y = DipY(py);

    if (HitTestThemeButton(x, y)) {
        SetTheme(!m_theme.dark);
        return;
    }
    if (HitTestClearButton(x, y)) {
        if (m_edit) SetWindowTextW(m_edit, L"");
        UpdateSearchResults();
        FocusSearch();
        return;
    }
    int pill = HitTestPill(x, y);
    if (pill >= 0) {
        const std::wstring values[3] = { L"全部", L"判断题", L"选择题" };
        if (m_typeFilter != values[pill]) {
            m_typeFilter = values[pill];
            UpdateSearchResults();
        }
        return;
    }
    if (HitTestSwitch(x, y)) {
        m_searchInOptions = !m_searchInOptions;
        UpdateSearchResults();
        return;
    }
    if (HitTestBackToTop(x, y)) {
        m_list.ScrollToTop();
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    if (m_list.HitTestScrollbar(x, y)) {
        m_list.BeginScrollbarDrag(x, y);
        SetCapture(m_hwnd);
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    if (HitTestField(x, y)) {
        FocusSearch();
        return;
    }

    int item = m_list.ItemIndexAt(x, y);
    if (item >= 0 && m_list.CopyButtonHit(item, x, y) && (size_t)item < m_search.items.size()) {
        CopyToClipboard(m_hwnd, m_search.items[item]->CopyText());
        m_list.MarkCopied(item);
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
}

void App::OnLButtonUp(int px, int py) {
    (void)px;
    (void)py;
    if (m_list.DraggingScrollbar()) {
        m_list.EndScrollbarDrag();
        ReleaseCapture();
    }
}

void App::OnMouseWheel(short delta) {
    float steps = (float)delta / 120.0f;
    m_list.ScrollBy(-steps * kWheelStep);
    StartAnimating();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void App::OnKeyDown(WPARAM key, bool ctrl) {
    if (ctrl && (key == 'F' || key == 'f')) {
        FocusSearch();
        return;
    }
    if (ctrl && (key == 'T' || key == 't')) {
        SetTheme(!m_theme.dark);
        return;
    }
    if (ctrl && key == VK_HOME) {
        m_list.ScrollToTop();
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    if (ctrl && key == VK_END) {
        m_list.ScrollToEnd();
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    switch (key) {
    case VK_ESCAPE:
        if (m_edit) SetWindowTextW(m_edit, L"");
        UpdateSearchResults();
        return;
    case VK_PRIOR:
        m_list.ScrollBy(-(m_panelRect.h - 80.0f));
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    case VK_NEXT:
        m_list.ScrollBy(m_panelRect.h - 80.0f);
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    case VK_HOME:
        m_list.ScrollToTop();
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    case VK_END:
        m_list.ScrollToEnd();
        StartAnimating();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    default:
        break;
    }
}

// ------------------------------------------------------------------ edit placeholder
void App::DrawEditPlaceholderIfNeeded() {
    if (!m_edit) return;
    if (GetWindowTextLengthW(m_edit) > 0) return;
    HDC hdc = GetDC(m_edit);
    if (!hdc) return;
    HFONT font = (HFONT)SendMessageW(m_edit, WM_GETFONT, 0, 0);
    HGDIOBJ oldFont = font ? SelectObject(hdc, font) : nullptr;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ToColorRef(m_theme.textMuted));
    RECT rc = { 0, 0, 0, 0 };
    GetClientRect(m_edit, &rc);
    const wchar_t* hint = L"搜索题目、选项关键词…";
    DrawTextW(hdc, hint, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (oldFont) SelectObject(hdc, oldFont);
    ReleaseDC(m_edit, hdc);
}

// ------------------------------------------------------------------ message loop
LRESULT CALLBACK App::EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = (App*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!app) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_KEYDOWN: {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if ((ctrl && (wParam == 'T' || wParam == 't')) || (ctrl && (wParam == 'F' || wParam == 'f')) ||
            (ctrl && (wParam == VK_HOME || wParam == VK_END)) || wParam == VK_PRIOR ||
            wParam == VK_NEXT || wParam == VK_ESCAPE) {
            app->OnKeyDown(wParam, ctrl);
            return 0;
        }
        break;
    }
    case WM_CHAR:
        if (wParam == VK_ESCAPE) return 0;
        break;
    case WM_PAINT: {
        LRESULT result = CallWindowProcW(app->m_editProc, hwnd, msg, wParam, lParam);
        app->DrawEditPlaceholderIfNeeded();
        return result;
    }
    case WM_MOUSEWHEEL:
        SendMessageW(app->m_hwnd, WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        InvalidateRect(app->m_hwnd, nullptr, FALSE);
        break;
    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        break;
    default:
        break;
    }
    return CallWindowProcW(app->m_editProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        app = (App*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)app);
        app->m_hwnd = hwnd;
    } else {
        app = (App*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }
    if (app) return app->HandleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        m_edit = CreateWindowExW(0, L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT, 0, 0, 100,
                                 24, hwnd, (HMENU)(INT_PTR)ID_SEARCH_EDIT, GetModuleHandleW(nullptr),
                                 nullptr);
        if (m_edit) {
            SetWindowLongPtrW(m_edit, GWLP_USERDATA, (LONG_PTR)this);
            m_editProc = (WNDPROC)SetWindowLongPtrW(m_edit, GWLP_WNDPROC, (LONG_PTR)App::EditProc);
            SendMessageW(m_edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
            SendMessageW(m_edit, EM_SETLIMITTEXT, 200, 0);
        }
        RebuildEditFont();
        UpdateEditColors();
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        Paint();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_PRINTCLIENT:
        Paint();
        return 0;
    case WM_SIZE:
        Layout();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_DPICHANGED: {
        UINT newDpi = HIWORD(wParam);
        RECT* target = (RECT*)lParam;
        if (target) {
            SetWindowPos(hwnd, nullptr, target->left, target->top, target->right - target->left,
                         target->bottom - target->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        ApplyDpi(newDpi);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO* info = (MINMAXINFO*)lParam;
        if (info) {
            info->ptMinTrackSize.x = DipToPixels(880.0f, m_dpi);
            info->ptMinTrackSize.y = DipToPixels(600.0f, m_dpi);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_NCMOUSEMOVE:
        break;
    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
        OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEWHEEL:
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_SETCURSOR: {
        if (m_list.DraggingScrollbar()) {
            SetCursor(LoadCursorW(nullptr, IDC_HAND));
            return TRUE;
        }
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt = {};
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            float x = DipX(pt.x);
            float y = DipY(pt.y);
            int item = m_list.ItemIndexAt(x, y);
            bool hand = HitTestPill(x, y) >= 0 || HitTestThemeButton(x, y) || HitTestClearButton(x, y) ||
                        HitTestSwitch(x, y) || HitTestBackToTop(x, y) || m_list.HitTestScrollbar(x, y) ||
                        (item >= 0 && m_list.CopyButtonHit(item, x, y)) || HitTestField(x, y);
            if (hand) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_SEARCH_EDIT && code == EN_CHANGE) {
            SetTimer(hwnd, kTimerSearch, kSearchDebounceMs, nullptr);
        }
        return 0;
    }
    case WM_TIMER:
        if (wParam == kTimerAnim) {
            AnimTick();
            return 0;
        }
        if (wParam == kTimerSearch) {
            KillTimer(hwnd, kTimerSearch);
            UpdateSearchResults();
            return 0;
        }
        if (wParam == kTimerPreload) {
            bool more = m_list.PremeasureNext(4.0);
            if (!more) KillTimer(hwnd, kTimerPreload);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC hdcEdit = (HDC)wParam;
        if (m_edit && (HWND)lParam == m_edit) {
            SetTextColor(hdcEdit, m_theme.editText);
            SetBkColor(hdcEdit, m_theme.editBg);
            if (!m_editBrush) m_editBrush = CreateSolidBrush(m_theme.editBg);
            return (LRESULT)m_editBrush;
        }
        break;
    }
    case WM_KEYDOWN:
        OnKeyDown(wParam, (GetKeyState(VK_CONTROL) & 0x8000) != 0);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, kTimerAnim);
        KillTimer(hwnd, kTimerSearch);
        KillTimer(hwnd, kTimerPreload);
        if (m_editFont) { DeleteObject(m_editFont); m_editFont = nullptr; }
        if (m_editBrush) { DeleteObject(m_editBrush); m_editBrush = nullptr; }
        m_gfx.Shutdown();
        m_fonts.Shutdown();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    int result = 0;
    {
        App app;
        if (app.Create(hInstance, nCmdShow)) result = app.Run();
    }
    CoUninitialize();
    return result;
}