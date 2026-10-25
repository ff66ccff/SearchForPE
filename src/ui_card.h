#pragma once
#include "common.h"
#include "theme.h"

struct CardLayout {
    Gdiplus::RectF rcCard;
    Gdiplus::RectF rcCopyQuestion;
    Gdiplus::RectF rcCopyAll;
    
    int copiedState = 0; // 0 = none, 1 = question, 2 = all
    std::chrono::steady_clock::time_point copiedTime;
    
    bool isHovered = false;
    bool isCopyQHovered = false;
    bool isCopyAllHovered = false;
};

class CardRenderer {
public:
    static void DrawRoundedRect(Gdiplus::Graphics& g, Gdiplus::Brush* brush, Gdiplus::Pen* pen, Gdiplus::RectF rect, float radius);
    
    static float MeasurePageCards(
        Gdiplus::Graphics& g,
        const std::vector<const Question*>& questions,
        size_t startIndex,
        size_t count,
        float cardX,
        float cardWidth,
        float startY,
        float gap,
        UINT dpi,
        std::vector<CardLayout>& outLayouts);

    static void RenderCard(
        Gdiplus::Graphics& g,
        const Question& q,
        size_t displayIndex,
        const CardLayout& layout,
        const std::vector<std::wstring>& keywords,
        const ColorScheme& theme,
        UINT dpi);
};
