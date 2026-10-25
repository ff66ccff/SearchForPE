#pragma once
#include "common.h"
#include "fonts.h"
#include "gfx.h"
#include "theme.h"

// A single, continuous, virtualised result list. There is deliberately no
// pagination anywhere: scrolling is the one and only navigation model.
class ResultList {
public:
    void Init(FontManager* fonts) { m_fonts = fonts; }

    void SetResults(std::vector<const Question*> items, std::vector<std::wstring> keywords);
    void Clear();

    void SetPanel(const RectF& panel);
    void SetContentWidth(float contentW);
    float ContentWidth() const { return m_contentW; }
    const RectF& Panel() const { return m_panel; }

    size_t Count() const { return m_items.size(); }
    bool Empty() const { return m_items.empty(); }
    bool AllLoaded() const { return m_loaded >= m_items.size(); }
    size_t LoadedCount() const { return m_loaded; }
    // Measures the next chunk regardless of the viewport (background preload so
    // the scrollbar extent becomes exact without blocking the UI).
    bool PremeasureNext(double budgetMs);

    float ContentHeight() const;          // exact height of measured rows
    float ContentExtent() const;          // measured + estimated remaining rows
    float MaxScroll() const;              // exact max scroll (measured rows only)
    float ExtentMaxScroll() const;        // estimated max scroll (stable while loading)
    void MeasureUntil(float contentY, double budgetMs);
    float ViewHeight() const { return (std::max)(0.0f, m_panel.h); }
    float ScrollY() const { return m_scroll; }
    float TargetScroll() const { return m_target; }

    void ScrollBy(float dy);
    void ScrollTo(float y, bool animate);
    void ScrollToEnd();
    void ScrollToTop();

    bool Animating() const;
    // Advances smooth scrolling / progressive loading / copy feedback.
    // Returns true when another frame should be painted.
    bool Tick(float dtMs);
    void EnsureLoaded();

    void Paint(Gfx& gfx, const Theme& theme);

    // ---- interaction -------------------------------------------------------
    int ItemIndexAt(float x, float y) const;     // row under point (panel coords)
    bool CopyButtonHit(int index, float x, float y) const;

    void SetHoverIndex(int index) { m_hoverIndex = index; }
    int HoveredIndex() const { return m_hoverIndex; }
    void SetHoveredCopy(bool on) { m_hoverCopy = on; }
    bool HoveredCopy() const { return m_hoverCopy; }

    bool ScrollbarVisible() const;
    bool HitTestScrollbar(float x, float y) const;
    float ScrollbarHitWidth() const { return 20.0f; }
    void BeginScrollbarDrag(float x, float y);
    void ScrollbarDragTo(float x, float y);
    void EndScrollbarDrag();
    bool DraggingScrollbar() const { return m_dragging; }
    bool ScrollbarHover() const { return m_scrollbarHover; }
    void SetScrollbarHover(bool on) { m_scrollbarHover = on; }

    void MarkCopied(int index);
    int CopiedIndex() const;
    void TickCopyFeedback(float dtMs);
    bool CopyFeedbackActive() const;

private:
    struct RowMetrics {
        float height = 0.0f;
        float questionH = 0.0f;
        std::vector<float> optionH;
    };

    void MeasureRow(size_t index, RowMetrics& out) const;
    void LoadMore(size_t maxRows, double budgetMs);
    void ClampTarget();
    int FindIndexAtContentY(float contentY) const;
    bool IsCorrectOption(const Question& q, const std::wstring& option) const;
    void DrawRow(Gfx& gfx, const Theme& theme, size_t index, float top) const;
    void DrawScrollbar(Gfx& gfx, const Theme& theme) const;
    float ThumbGeometry(float* outThumbY, float* outThumbH) const;  // returns track top

    FontManager* m_fonts = nullptr;

    std::vector<const Question*> m_items;
    std::vector<std::wstring> m_keywords;

    RectF m_panel;
    float m_contentW = 0.0f;

    std::vector<float> m_heights;   // measured row heights
    mutable std::unordered_map<int, RowMetrics> m_metricsCache;  // question id -> metrics
    std::vector<float> m_tops;      // m_tops[i] = y offset of row i (size = loaded + 1)

    size_t m_loaded = 0;

    float m_scroll = 0.0f;
    float m_target = 0.0f;

    bool m_endRequested = false;
    int m_hoverIndex = -1;
    bool m_hoverCopy = false;
    bool m_scrollbarHover = false;
    bool m_dragging = false;
    float m_dragGrabOffset = 0.0f;
    bool m_dragExtentFrozen = false;
    float m_dragExtent = 0.0f;
    float m_dragTrackTop = 0.0f;
    float m_dragTrackHeight = 0.0f;
    float m_dragThumbHeight = 0.0f;

    int m_copiedIndex = -1;
    float m_copiedAge = 0.0f;
    float m_copyToastAge = -1.0f;
};