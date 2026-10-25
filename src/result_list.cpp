#include "result_list.h"

#include <chrono>

namespace {

constexpr float kPadX          = 20.0f;
constexpr float kPadTop        = 18.0f;
constexpr float kPadBottom     = 18.0f;
constexpr float kMetaH         = 22.0f;
constexpr float kStemGap       = 10.0f;
constexpr float kOptionsGap    = 8.0f;
constexpr float kOptionGap     = 3.0f;
constexpr float kIndexW        = 26.0f;
constexpr float kIndexGap      = 8.0f;
constexpr float kDotR          = 2.5f;
constexpr float kDotGap        = 6.0f;
constexpr float kChipsGap      = 10.0f;
constexpr float kChipH         = 17.0f;
constexpr float kChipPadX      = 6.0f;
constexpr float kChipGap       = 6.0f;
constexpr float kPillH         = 22.0f;
constexpr float kPillPadX      = 8.0f;
constexpr float kPillIconGap   = 4.0f;
constexpr float kPillIcon      = 12.0f;
constexpr float kCopyBtnSize   = 24.0f;
constexpr float kMetaRightGap  = 8.0f;
constexpr float kCopiedGap     = 10.0f;
constexpr float kScrollbarW    = 6.0f;
constexpr float kScrollbarRight = 6.0f;
constexpr float kScrollbarInsetV = 14.0f;
constexpr float kPanelRadius   = 14.0f;
constexpr float kPreload       = 480.0f;
constexpr double kLoadBudgetMs = 5.0;
constexpr float kCopyFeedbackSeconds = 1.2f;

struct OptionSplit {
    std::wstring prefix;
    std::wstring body;
    float prefixW = 0.0f;
};

// Splits "A．节奏" into ("A．", "节奏") so wrapped option text can hang under
// the body instead of under the letter marker.
OptionSplit SplitOption(const FontManager& fonts, const std::wstring& text) {
    OptionSplit split;
    split.body = text;
    if (text.empty()) return split;
    wchar_t first = text[0];
    bool letter = (first >= L'A' && first <= L'Z') || (first >= L'a' && first <= L'z');
    if (!letter) return split;

    size_t i = 1;
    while (i < text.size()) {
        wchar_t c = text[i];
        bool sep = (c == L'.' || c == L'\uFF0E' || c == L'\u3001' || c == L':' || c == L'\uFF1A' ||
                    c == L')' || c == L'\uFF09' || c == L' ' || c == L'\t' || c == L'\u3000');
        if (!sep) break;
        ++i;
    }
    if (i <= 1) return split;               // "Axx" — do not treat as marker
    if (i >= text.size()) return split;     // nothing but the marker
    split.prefix = text.substr(0, i);
    split.body = text.substr(i);
    split.prefixW = fonts.MeasureWidth(split.prefix, TextRole::Option);
    return split;
}

}  // namespace

// ------------------------------------------------------------------ data
void ResultList::SetResults(std::vector<const Question*> items, std::vector<std::wstring> keywords) {
    m_items = std::move(items);
    m_keywords = std::move(keywords);
    m_heights.clear();
    m_tops.clear();
    m_tops.push_back(0.0f);
    m_loaded = 0;
    m_scroll = 0.0f;
    m_target = 0.0f;
    m_endRequested = false;
    m_hoverIndex = -1;
    m_hoverCopy = false;
    m_copiedIndex = -1;
    m_copiedAge = 0.0f;
    m_dragging = false;
    EnsureLoaded();
}

void ResultList::Clear() {
    SetResults({}, {});
}

void ResultList::SetPanel(const RectF& panel) {
    if (panel.x == m_panel.x && panel.y == m_panel.y && panel.w == m_panel.w && panel.h == m_panel.h) {
        return;
    }
    m_panel = panel;
    ClampTarget();
}

void ResultList::SetContentWidth(float contentW) {
    if (std::fabs(contentW - m_contentW) < 0.01f) return;
    m_contentW = contentW;
    m_metricsCache.clear();
    m_heights.clear();
    m_tops.clear();
    m_tops.push_back(0.0f);
    m_loaded = 0;
    m_scroll = 0.0f;
    m_target = 0.0f;
    EnsureLoaded();
}

// ------------------------------------------------------------------ metrics
void ResultList::MeasureRow(size_t index, RowMetrics& out) const {
    out = RowMetrics();
    if (index >= m_items.size() || !m_fonts) return;
    const Question& q = *m_items[index];
    auto cached = m_metricsCache.find(q.id);
    if (cached != m_metricsCache.end()) {
        out = cached->second;
        return;
    }
    const float textW = m_contentW;

    out.questionH = m_fonts->MeasureHeight(q.issue, TextRole::Question, textW);
    float optionsH = 0.0f;
    if (!q.options.empty()) {
        out.optionH.reserve(q.options.size());
        for (size_t i = 0; i < q.options.size(); ++i) {
            OptionSplit split = SplitOption(*m_fonts, q.options[i]);
            float bodyW = (std::max)(40.0f, textW - split.prefixW);
            float bodyH = m_fonts->MeasureHeight(split.body, TextRole::Option, bodyW);
            out.optionH.push_back(bodyH);
            optionsH += bodyH;
            if (i + 1 < q.options.size()) optionsH += kOptionGap;
        }
    }

    float height = kPadTop + kMetaH + kStemGap + out.questionH;
    if (!q.options.empty()) height += kOptionsGap + optionsH;
    out.height = height + kPadBottom;
    m_metricsCache[q.id] = out;
}

bool ResultList::PremeasureNext(double budgetMs) {
    if (!m_fonts || m_items.empty() || m_contentW <= 1.0f) return false;
    if (AllLoaded()) return false;
    LoadMore(256, budgetMs);
    ClampTarget();
    return !AllLoaded();
}

void ResultList::LoadMore(size_t maxRows, double budgetMs) {
    if (!m_fonts || m_items.empty() || m_contentW <= 1.0f) return;
    auto start = std::chrono::steady_clock::now();
    size_t rows = 0;
    while (m_loaded < m_items.size() && rows < maxRows) {
        double elapsed = std::chrono::duration<double, std::milli>(
                             std::chrono::steady_clock::now() - start).count();
        if (rows >= 6 && elapsed > budgetMs) break;
        RowMetrics metrics;
        MeasureRow(m_loaded, metrics);
        m_heights.push_back(metrics.height);
        m_tops.push_back(m_tops.back() + metrics.height);
        ++m_loaded;
        ++rows;
    }
}

void ResultList::EnsureLoaded() {
    if (m_items.empty() || !m_fonts || m_contentW <= 1.0f) return;
    const float need = (std::max)(m_scroll, m_target) + ViewHeight() + kPreload;
    while (m_loaded < m_items.size() && m_tops[m_loaded] < need) {
        LoadMore(16, kLoadBudgetMs);
    }
    if (m_endRequested && m_loaded >= m_items.size()) {
        m_target = MaxScroll();
    }
    ClampTarget();
}

float ResultList::ContentHeight() const {
    return m_tops.empty() ? 0.0f : m_tops.back();
}

float ResultList::ContentExtent() const {
    if (m_loaded == 0) return 0.0f;
    if (m_loaded >= m_items.size()) return m_tops.back();
    // Extrapolate the unmeasured tail from the average measured row height so the
    // scrollbar keeps a stable size and drag mapping while rows stream in.
    const float measured = m_tops.back();
    const float average = measured / (float)m_loaded;
    return measured + average * (float)(m_items.size() - m_loaded);
}

float ResultList::MaxScroll() const {
    return (std::max)(0.0f, ContentHeight() - ViewHeight());
}

float ResultList::ExtentMaxScroll() const {
    return (std::max)(0.0f, ContentExtent() - ViewHeight());
}

void ResultList::MeasureUntil(float contentY, double budgetMs) {
    if (!m_fonts || m_items.empty() || m_contentW <= 1.0f) return;
    auto start = std::chrono::steady_clock::now();
    while (m_loaded < m_items.size() && m_tops[m_loaded] < contentY) {
        LoadMore(64, budgetMs);
        double elapsed = std::chrono::duration<double, std::milli>(
                             std::chrono::steady_clock::now() - start).count();
        if (elapsed > budgetMs) break;
    }
    ClampTarget();
}

// ------------------------------------------------------------------ scrolling
void ResultList::ClampTarget() {
    const float extentMax = ExtentMaxScroll();
    if (m_target > extentMax + 1.0f || m_target < -1.0f) {
    }
    m_target = Clampf(m_target, 0.0f, extentMax);
    if (m_endRequested) m_target = extentMax;
}

void ResultList::ScrollBy(float dy) {
    m_endRequested = false;
    m_target = Clampf(m_target + dy, 0.0f, ExtentMaxScroll());
    EnsureLoaded();
}

void ResultList::ScrollTo(float y, bool animate) {
    m_endRequested = false;
    m_target = Clampf(y, 0.0f, ExtentMaxScroll());
    if (!animate) m_scroll = m_target;
    EnsureLoaded();
}

void ResultList::ScrollToEnd() {
    if (m_items.empty()) return;
    m_endRequested = true;
    EnsureLoaded();
    m_target = ExtentMaxScroll();
}

void ResultList::ScrollToTop() {
    m_endRequested = false;
    m_target = 0.0f;
}

bool ResultList::Animating() const {
    return std::fabs(m_target - m_scroll) > 0.15f || m_endRequested;
}

bool ResultList::Tick(float dtMs) {
    bool needsPaint = false;
    size_t before = m_loaded;

    if (m_endRequested) {
        EnsureLoaded();
        m_target = ExtentMaxScroll();
        if (m_loaded >= m_items.size()) m_endRequested = false;
        needsPaint = true;
    } else {
        EnsureLoaded();
    }

    if (std::fabs(m_target - m_scroll) > 0.15f) {
        float k = 1.0f - expf(-dtMs / 52.0f);
        m_scroll += (m_target - m_scroll) * k;
        if (std::fabs(m_target - m_scroll) < 0.3f) m_scroll = m_target;
        needsPaint = true;
    } else if (m_scroll != m_target) {
        m_scroll = m_target;
    }

    if (m_loaded != before) {
        ClampTarget();
        needsPaint = true;
    }
    return needsPaint;
}

// ------------------------------------------------------------------ querying
int ResultList::FindIndexAtContentY(float contentY) const {
    if (m_loaded == 0) return -1;
    auto it = std::upper_bound(m_tops.begin(), m_tops.begin() + (size_t)m_loaded + 1, contentY);
    if (it == m_tops.begin()) return -1;
    int index = (int)(it - m_tops.begin()) - 1;
    if (index < 0 || (size_t)index >= m_loaded) return -1;
    return index;
}

int ResultList::ItemIndexAt(float x, float y) const {
    if (m_items.empty() || !m_panel.Contains(x, y)) return -1;
    if (ScrollbarVisible() && x >= m_panel.Right() - kScrollbarRight - kScrollbarW - 4.0f) return -1;
    float contentY = y - m_panel.y + m_scroll;
    return FindIndexAtContentY(contentY);
}

bool ResultList::CopyButtonHit(int index, float x, float y) const {
    if (index < 0 || (size_t)index >= m_loaded) return false;
    float top = m_panel.y + m_tops[index] - m_scroll;
    float metaY = top + kPadTop;
    float copyX = m_panel.Right() - kPadX - kCopyBtnSize;
    float copyY = metaY + (kMetaH - kCopyBtnSize) * 0.5f;
    return RectF(copyX, copyY, kCopyBtnSize, kCopyBtnSize).Contains(x, y);
}

bool ResultList::ScrollbarVisible() const {
    return !m_items.empty() && ContentHeight() > ViewHeight() + 1.0f;
}

float ResultList::ThumbGeometry(float* outThumbY, float* outThumbH) const {
    float trackTop = m_panel.y + kScrollbarInsetV;
    float trackH = (std::max)(0.0f, m_panel.h - kScrollbarInsetV * 2.0f);
    float contentH = ContentExtent();
    float viewH = ViewHeight();
    float thumbH = trackH;
    if (contentH > viewH + 1.0f && contentH > 0.0f) {
        thumbH = (std::max)(36.0f, trackH * viewH / contentH);
    }
    float travel = trackH - thumbH;
    float extentMax = ExtentMaxScroll();
    float ratio = extentMax > 0.0f ? Clampf(m_scroll / extentMax, 0.0f, 1.0f) : 0.0f;
    if (outThumbH) *outThumbH = thumbH;
    if (outThumbY) *outThumbY = trackTop + travel * ratio;
    return trackTop;
}

bool ResultList::HitTestScrollbar(float x, float y) const {
    if (!ScrollbarVisible()) return false;
    if (y < m_panel.y || y > m_panel.Bottom()) return false;
    return x >= m_panel.Right() - kScrollbarRight - kScrollbarW - ScrollbarHitWidth() &&
           x <= m_panel.Right();
}

void ResultList::BeginScrollbarDrag(float x, float y) {
    if (!ScrollbarVisible()) return;
    {
        float ty = 0, th = 0;
        ThumbGeometry(&ty, &th);
    }
    m_dragExtentFrozen = false;
    float thumbY = 0.0f, thumbH = 0.0f;
    ThumbGeometry(&thumbY, &thumbH);
    m_dragging = true;
    if (y >= thumbY && y <= thumbY + thumbH) {
        m_dragGrabOffset = y - thumbY;
    } else {
        m_dragGrabOffset = thumbH * 0.5f;
        ScrollbarDragTo(x, y);
    }
}

void ResultList::ScrollbarDragTo(float x, float y) {
    if (!ScrollbarVisible()) return;
    (void)x;
    // Freeze the mapping for the duration of the drag: the row set under the
    // thumb may still be streaming in, and recomputing the extent every move
    // would make the thumb stutter.
    if (!m_dragExtentFrozen) {
        m_dragExtent = ExtentMaxScroll();
        m_dragTrackTop = m_panel.y + kScrollbarInsetV;
        m_dragTrackHeight = (std::max)(0.0f, m_panel.h - kScrollbarInsetV * 2.0f);
        float thumbY = 0.0f, thumbH = 0.0f;
        ThumbGeometry(&thumbY, &thumbH);
        m_dragThumbHeight = thumbH;
        m_dragExtentFrozen = true;
    }
    float travel = m_dragTrackHeight - m_dragThumbHeight;
    if (travel <= 0.0f) return;
    float newThumbY = Clampf(y - m_dragGrabOffset, m_dragTrackTop, m_dragTrackTop + travel);
    float ratio = Clampf((newThumbY - m_dragTrackTop) / travel, 0.0f, 1.0f);
    m_endRequested = false;
    m_scroll = m_target = ratio * m_dragExtent;
    // Pull the rows under the viewport into the measured set right away so the
    // drag never reveals an unmeasured (blank) stretch.
    MeasureUntil(m_scroll + ViewHeight() + 240.0f, 8.0);
    EnsureLoaded();
}

void ResultList::EndScrollbarDrag() {
    m_dragging = false;
    m_dragExtentFrozen = false;
}

void ResultList::MarkCopied(int index) {
    m_copiedIndex = index;
    m_copiedAge = 0.0f;
}

int ResultList::CopiedIndex() const {
    return CopyFeedbackActive() ? m_copiedIndex : -1;
}

bool ResultList::CopyFeedbackActive() const {
    return m_copiedIndex >= 0 && m_copiedAge < kCopyFeedbackSeconds;
}

void ResultList::TickCopyFeedback(float dtMs) {
    if (m_copiedIndex < 0) return;
    m_copiedAge += dtMs * 0.001f;
    if (m_copiedAge >= kCopyFeedbackSeconds) m_copiedIndex = -1;
}

// ------------------------------------------------------------------ paint
bool ResultList::IsCorrectOption(const Question& q, const std::wstring& option) const {
    if (option.empty()) return false;
    wchar_t first = option[0];
    if (first >= L'a' && first <= L'z') first = (wchar_t)(first - L'a' + L'A');
    if (first < L'A' || first > L'Z') return false;
    return q.answer.find(first) != std::wstring::npos;
}

void ResultList::Paint(Gfx& gfx, const Theme& theme) {
    gfx.FillRound(m_panel, kPanelRadius, theme.surface);
    gfx.StrokeRound(m_panel.Inset(0.5f, 0.5f), kPanelRadius, theme.border, 1.0f);

    if (m_items.empty() || !m_fonts) return;

    gfx.PushRoundClip(m_panel.Inset(1.0f, 1.0f), kPanelRadius - 1.0f);

    int start = FindIndexAtContentY(m_scroll);
    if (start < 0) start = 0;
    size_t index = (size_t)start;
    while (index < m_loaded) {
        float top = m_panel.y + m_tops[index] - m_scroll;
        if (top > m_panel.Bottom()) break;
        DrawRow(gfx, theme, index, top);
        index++;
    }

    gfx.PopRoundClip();

    if (ScrollbarVisible()) DrawScrollbar(gfx, theme);
}

void ResultList::DrawScrollbar(Gfx& gfx, const Theme& theme) const {
    float thumbY = 0.0f, thumbH = 0.0f;
    ThumbGeometry(&thumbY, &thumbH);
    bool hot = m_scrollbarHover || m_dragging;
    Color color = hot ? theme.scrollThumbHot : theme.scrollThumb;
    gfx.FillRound(RectF(m_panel.Right() - kScrollbarRight - kScrollbarW, thumbY, kScrollbarW, thumbH),
                  kScrollbarW * 0.5f, color);
}

void ResultList::DrawRow(Gfx& gfx, const Theme& theme, size_t index, float top) const {
    if (index >= m_items.size() || !m_fonts) return;
    const Question& q = *m_items[index];
    const float left = m_panel.x + kPadX;
    const float right = m_panel.Right() - kPadX;
    const float textW = m_contentW;
    const bool hovered = (int)index == m_hoverIndex;
    const bool copied = CopyFeedbackActive() && (int)index == m_copiedIndex;

    if (hovered && !copied) {
        gfx.FillRect(RectF(m_panel.x + 1.0f, top, m_panel.w - 2.0f, m_heights[index]), theme.surfaceHover);
    } else if (copied) {
        gfx.FillRect(RectF(m_panel.x + 1.0f, top, m_panel.w - 2.0f, m_heights[index]), theme.surfaceHover);
    }

    const float metaY = top + kPadTop;

    // ---- index
    wchar_t indexBuf[24];
    if (index + 1 < 100) swprintf(indexBuf, 24, L"%02zu", index + 1);
    else swprintf(indexBuf, 24, L"%zu", index + 1);
    IDWriteTextLayout* indexLayout = m_fonts->MakeLayout(indexBuf, TextRole::Meta, 200.0f, false);
    if (indexLayout) {
        DWRITE_TEXT_METRICS m = {};
        indexLayout->GetMetrics(&m);
        gfx.DrawTextLayout(indexLayout, left, metaY + (kMetaH - m.height) * 0.5f, theme.textMuted);
        indexLayout->Release();
    }

    // ---- type dot + label
    float x = left + kIndexW + kIndexGap;
    Color dotColor = q.IsJudge() ? theme.dotJudge : (q.IsMulti() ? theme.dotMulti : theme.dotSingle);
    gfx.FillCircle(x + kDotR, metaY + kMetaH * 0.5f, kDotR, dotColor);
    x += kDotR * 2.0f + kDotGap;

    IDWriteTextLayout* typeLayout = m_fonts->MakeLayout(q.type, TextRole::Meta, 200.0f, false);
    if (typeLayout) {
        DWRITE_TEXT_METRICS m = {};
        typeLayout->GetMetrics(&m);
        gfx.DrawTextLayout(typeLayout, x, metaY + (kMetaH - m.height) * 0.5f, theme.textSecondary);
        x += m.width + kChipsGap;
        typeLayout->Release();
    }

    // ---- matched keyword chips
    int chips = 0;
    if (!m_keywords.empty()) {
        for (const auto& kw : m_keywords) {
            if (chips >= 2) break;
            if (kw.empty()) continue;
            if (q.issue_lower.find(kw) == std::wstring::npos &&
                q.options_lower_combined.find(kw) == std::wstring::npos) {
                continue;
            }
            float chipTextW = m_fonts->MeasureWidth(kw, TextRole::MetaMedium);
            float chipW = chipTextW + kChipPadX * 2.0f;
            if (x + chipW > right - 260.0f) break;
            float chipY = metaY + (kMetaH - kChipH) * 0.5f;
            gfx.FillRound(RectF(x, chipY, chipW, kChipH), 4.0f, theme.chipBg);
            IDWriteTextLayout* chipLayout = m_fonts->MakeLayout(kw, TextRole::MetaMedium, chipW, false);
            if (chipLayout) {
                DWRITE_TEXT_METRICS m = {};
                chipLayout->GetMetrics(&m);
                gfx.DrawTextLayout(chipLayout, x + kChipPadX, chipY + (kChipH - m.height) * 0.5f,
                                   theme.chipText);
                chipLayout->Release();
            }
            x += chipW + kChipGap;
            ++chips;
        }
    }

    // ---- right cluster: [已复制] [answer pill] [copy button]
    const float copyX = right - kCopyBtnSize;
    const float copyY = metaY + (kMetaH - kCopyBtnSize) * 0.5f;

    std::wstring answerText;
    Color pillBg = theme.accentSoft;
    Color pillFg = theme.accent;
    bool judgeIcon = q.IsJudge();
    bool judgeTrue = q.IsTrueAnswer();
    if (q.IsJudge()) {
        answerText = q.IsTrueAnswer() ? L"正确" : L"错误";
        pillBg = judgeTrue ? theme.successSoft : theme.dangerSoft;
        pillFg = judgeTrue ? theme.success : theme.danger;
    } else {
        answerText = L"答案 " + q.AnswerText();
    }

    float answerTextW = m_fonts->MeasureWidth(answerText, TextRole::Pill);
    float iconW = judgeIcon ? kPillIcon + kPillIconGap : 0.0f;
    float pillW = answerTextW + iconW + kPillPadX * 2.0f;
    float pillX = copyX - kMetaRightGap - pillW;
    float pillY = metaY + (kMetaH - kPillH) * 0.5f;
    gfx.FillRound(RectF(pillX, pillY, pillW, kPillH), 6.0f, pillBg);
    if (judgeIcon) {
        RectF iconBox(pillX + kPillPadX, pillY + (kPillH - kPillIcon) * 0.5f, kPillIcon, kPillIcon);
        gfx.DrawIcon(judgeTrue ? Icon::Check : Icon::Cross, iconBox, pillFg, 1.6f);
    }
    IDWriteTextLayout* answerLayout = m_fonts->MakeLayout(answerText, TextRole::Pill, pillW, false);
    if (answerLayout) {
        DWRITE_TEXT_METRICS m = {};
        answerLayout->GetMetrics(&m);
        gfx.DrawTextLayout(answerLayout, pillX + kPillPadX + iconW, pillY + (kPillH - m.height) * 0.5f,
                           pillFg);
        answerLayout->Release();
    }

    if (copied) {
        const std::wstring copiedText = L"已复制";
        float copiedW = m_fonts->MeasureWidth(copiedText, TextRole::Status);
        float copiedX = pillX - kCopiedGap - copiedW;
        IDWriteTextLayout* copiedLayout = m_fonts->MakeLayout(copiedText, TextRole::Status, copiedW, false);
        if (copiedLayout) {
            DWRITE_TEXT_METRICS m = {};
            copiedLayout->GetMetrics(&m);
            gfx.DrawTextLayout(copiedLayout, copiedX, metaY + (kMetaH - m.height) * 0.5f,
                               theme.textMuted);
            copiedLayout->Release();
        }
        RectF checkBox(copyX + 4.5f, copyY + 4.5f, 15.0f, 15.0f);
        gfx.DrawIcon(Icon::Check, checkBox, theme.success, 1.7f);
    } else if (hovered) {
        if (m_hoverCopy) {
            gfx.FillRound(RectF(copyX, copyY, kCopyBtnSize, kCopyBtnSize), 6.0f, theme.controlHover);
        }
        RectF iconBox(copyX + 4.5f, copyY + 4.5f, 15.0f, 15.0f);
        gfx.DrawIcon(Icon::Copy, iconBox, m_hoverCopy ? theme.textPrimary : theme.textMuted, 1.5f);
    }

    // ---- question stem
    float y = metaY + kMetaH + kStemGap;
    IDWriteTextLayout* questionLayout = m_fonts->MakeLayout(q.issue, TextRole::Question, textW);
    if (questionLayout) {
        DWRITE_TEXT_METRICS m = {};
        questionLayout->GetMetrics(&m);
        gfx.DrawTextLayout(questionLayout, left, y, theme.textPrimary);
        y += m.height;
        questionLayout->Release();
    } else {
        y += 25.5f;
    }

    // ---- options (hanging indent so wrapped text aligns under the body)
    if (!q.options.empty()) {
        y += kOptionsGap;
        for (size_t oi = 0; oi < q.options.size(); ++oi) {
            const std::wstring& option = q.options[oi];
            bool correct = IsCorrectOption(q, option);
            Color optionColor = correct ? theme.success : theme.textSecondary;
            TextRole bodyRole = correct ? TextRole::OptionStrong : TextRole::Option;
            OptionSplit split = SplitOption(*m_fonts, option);
            float bodyW = (std::max)(40.0f, textW - split.prefixW);

            float lineH = 0.0f;
            if (!split.prefix.empty()) {
                IDWriteTextLayout* keyLayout =
                    m_fonts->MakeLayout(split.prefix, bodyRole, split.prefixW + 4.0f, false);
                if (keyLayout) {
                    gfx.DrawTextLayout(keyLayout, left, y, optionColor);
                    keyLayout->Release();
                }
            }
            IDWriteTextLayout* bodyLayout = m_fonts->MakeLayout(split.body, bodyRole, bodyW);
            if (bodyLayout) {
                DWRITE_TEXT_METRICS m = {};
                bodyLayout->GetMetrics(&m);
                gfx.DrawTextLayout(bodyLayout, left + split.prefixW, y, optionColor);
                lineH = m.height;
                bodyLayout->Release();
            } else {
                lineH = 21.6f;
            }
            if (oi + 1 < q.options.size()) y += lineH + kOptionGap;
            else y += lineH;
        }
    }

    // ---- divider (top edge of the next row)
    if (index + 1 < m_items.size()) {
        float dividerY = top + m_heights[index] - 0.5f;
        gfx.Line(left, dividerY, right, dividerY, theme.divider, 1.0f);
    }
}