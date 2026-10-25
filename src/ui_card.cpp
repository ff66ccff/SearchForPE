#include "ui_card.h"

using namespace Gdiplus;

void CardRenderer::DrawRoundedRect(Graphics& g, Brush* brush, Pen* pen, RectF rect, float radius) {
    if (radius <= 0.0f) {
        if (brush) g.FillRectangle(brush, rect);
        if (pen) g.DrawRectangle(pen, rect);
        return;
    }
    float d = radius * 2.0f;
    if (d > rect.Width) d = rect.Width;
    if (d > rect.Height) d = rect.Height;

    GraphicsPath path;
    path.AddArc(rect.X, rect.Y, d, d, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();

    if (brush) g.FillPath(brush, &path);
    if (pen) g.DrawPath(pen, &path);
}

float CardRenderer::MeasurePageCards(
    Graphics& g,
    const std::vector<const Question*>& questions,
    size_t startIndex,
    size_t count,
    float cardX,
    float cardWidth,
    float startY,
    float gap,
    UINT dpi,
    std::vector<CardLayout>& outLayouts)
{
    outLayouts.clear();
    outLayouts.resize(count);

    float curY = startY;
    float padH = (float)Scale(20, dpi);
    float padV = (float)Scale(16, dpi);
    float headerH = (float)Scale(28, dpi);
    float contentW = cardWidth - (padH * 2.0f);

    Font fontQuestion(L"Microsoft YaHei UI", (float)Scale(13, dpi), FontStyleRegular, UnitPixel);
    Font fontOption(L"Microsoft YaHei UI", (float)Scale(11, dpi), FontStyleRegular, UnitPixel);
    StringFormat sf(StringFormat::GenericTypographic());

    for (size_t i = 0; i < count; ++i) {
        size_t idx = startIndex + i;
        if (idx >= questions.size()) break;
        const Question* q = questions[idx];

        CardLayout& layout = outLayouts[i];
        float cardTop = curY;
        float innerY = cardTop + padV + headerH + (float)Scale(10, dpi);

        // Measure Question Text
        RectF layoutQ(cardX + padH, innerY, contentW, 10000.0f);
        RectF boundQ;
        g.MeasureString(q->issue.c_str(), -1, &fontQuestion, layoutQ, &sf, &boundQ);
        innerY += boundQ.Height + (float)Scale(12, dpi);

        // Measure Options
        if (!q->options.empty()) {
            float optInnerPadV = (float)Scale(10, dpi);
            float optInnerPadH = (float)Scale(14, dpi);
            float optContentW = contentW - (optInnerPadH * 2.0f);
            float optTotalH = optInnerPadV * 2.0f;

            for (const auto& opt : q->options) {
                RectF layoutOpt(0, 0, optContentW, 10000.0f);
                RectF boundOpt;
                g.MeasureString(opt.c_str(), -1, &fontOption, layoutOpt, &sf, &boundOpt);
                optTotalH += boundOpt.Height + (float)Scale(6, dpi);
            }
            innerY += optTotalH + (float)Scale(12, dpi);
        }

        // Measure Answer Box
        float answerH = (float)Scale(36, dpi);
        innerY += answerH;

        // Card bottom padding
        float totalCardH = (innerY + padV) - cardTop;
        layout.rcCard = RectF(cardX, cardTop, cardWidth, totalCardH);

        // Position Copy Buttons at Card Header right
        float btnW = (float)Scale(76, dpi);
        float btnH = (float)Scale(24, dpi);
        float btnRight = cardX + cardWidth - padH;
        float btnY = cardTop + padV;

        layout.rcCopyAll = RectF(btnRight - btnW, btnY, btnW, btnH);
        layout.rcCopyQuestion = RectF(btnRight - (btnW * 2.0f) - (float)Scale(8, dpi), btnY, btnW, btnH);

        curY += totalCardH + gap;
    }

    return curY;
}

void CardRenderer::RenderCard(
    Graphics& g,
    const Question& q,
    size_t displayIndex,
    const CardLayout& layout,
    const std::vector<std::wstring>& keywords,
    const ColorScheme& theme,
    UINT dpi)
{
    float radius = (float)Scale(10, dpi);
    float padH = (float)Scale(20, dpi);
    float padV = (float)Scale(16, dpi);

    // 1. Draw Card Background & Border
    SolidBrush brushCard(layout.isHovered ? theme.bgCardHover : theme.bgCard);
    Pen penCard(layout.isHovered ? theme.borderCardHover : theme.borderCard, 1.0f);
    DrawRoundedRect(g, &brushCard, &penCard, layout.rcCard, radius);

    // 2. Header Area
    float headerY = layout.rcCard.Y + padV;
    float curHeaderX = layout.rcCard.X + padH;

    // Type Badge
    Color badgeBg = (q.type == L"判断题") ? theme.badgeJudgeBg : 
                    ((q.type == L"多选题") ? theme.badgeMultiBg : theme.badgeSingleBg);
    Color badgeText = (q.type == L"判断题") ? theme.badgeJudgeText : 
                      ((q.type == L"多选题") ? theme.badgeMultiText : theme.badgeSingleText);
    Color badgeBorder = (q.type == L"判断题") ? theme.badgeJudgeBorder : 
                        ((q.type == L"多选题") ? theme.badgeMultiBorder : theme.badgeSingleBorder);

    SolidBrush brushBadge(badgeBg);
    Pen penBadge(badgeBorder, 1.0f);
    SolidBrush brushBadgeText(badgeText);
    Font fontBadge(L"Microsoft YaHei UI", (float)Scale(10, dpi), FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    float badgeW = (float)Scale(58, dpi);
    float badgeH = (float)Scale(22, dpi);
    RectF rcBadge(curHeaderX, headerY + (float)Scale(1, dpi), badgeW, badgeH);
    DrawRoundedRect(g, &brushBadge, &penBadge, rcBadge, (float)Scale(4, dpi));
    g.DrawString(q.type.c_str(), -1, &fontBadge, rcBadge, &sfCenter, &brushBadgeText);

    curHeaderX += badgeW + (float)Scale(10, dpi);

    // Question Number (#Index)
    std::wstring indexStr = L"#" + std::to_wstring(displayIndex);
    SolidBrush brushIndex(theme.textMuted);
    Font fontIndex(L"Microsoft YaHei UI", (float)Scale(11, dpi), FontStyleBold, UnitPixel);
    StringFormat sfLeftVCenter;
    sfLeftVCenter.SetAlignment(StringAlignmentNear);
    sfLeftVCenter.SetLineAlignment(StringAlignmentCenter);

    RectF rcIndex(curHeaderX, headerY, (float)Scale(70, dpi), (float)Scale(24, dpi));
    g.DrawString(indexStr.c_str(), -1, &fontIndex, rcIndex, &sfLeftVCenter, &brushIndex);

    // Matched Keyword Tags in Header
    std::vector<std::wstring> matchedKw;
    for (const auto& kw : keywords) {
        if (!kw.empty() && (q.issue_lower.find(kw) != std::wstring::npos || q.options_lower_combined.find(kw) != std::wstring::npos)) {
            matchedKw.push_back(kw);
        }
    }
    if (!matchedKw.empty()) {
        float kwTagX = curHeaderX + (float)Scale(65, dpi);
        SolidBrush brushKwBg(theme.highlightBg);
        SolidBrush brushKwText(theme.highlightText);
        Font fontKw(L"Microsoft YaHei UI", (float)Scale(10, dpi), FontStyleRegular, UnitPixel);

        for (const auto& kw : matchedKw) {
            std::wstring kwTag = L"匹配: " + kw;
            RectF boundKw;
            g.MeasureString(kwTag.c_str(), -1, &fontKw, PointF(0, 0), &sfLeftVCenter, &boundKw);
            float tagW = boundKw.Width + (float)Scale(12, dpi);
            float tagH = (float)Scale(20, dpi);

            if (kwTagX + tagW < layout.rcCopyQuestion.X - (float)Scale(10, dpi)) {
                RectF rcKw(kwTagX, headerY + (float)Scale(2, dpi), tagW, tagH);
                DrawRoundedRect(g, &brushKwBg, nullptr, rcKw, (float)Scale(4, dpi));
                g.DrawString(kwTag.c_str(), -1, &fontKw, rcKw, &sfCenter, &brushKwText);
                kwTagX += tagW + (float)Scale(6, dpi);
            }
        }
    }

    // Copy Buttons
    auto DrawBtn = [&](const RectF& rc, const wchar_t* text, bool hovered, bool active) {
        SolidBrush btnBg(active ? theme.badgeJudgeBg : (hovered ? theme.btnActionHoverBg : theme.btnActionBg));
        Pen btnBorder(active ? theme.badgeJudgeBorder : (hovered ? theme.borderCardHover : theme.btnActionBorder), 1.0f);
        SolidBrush btnText(active ? theme.badgeJudgeText : (hovered ? theme.textPrimary : theme.btnActionText));
        Font fontBtn(L"Microsoft YaHei UI", (float)Scale(10, dpi), FontStyleRegular, UnitPixel);

        DrawRoundedRect(g, &btnBg, &btnBorder, rc, (float)Scale(4, dpi));
        g.DrawString(text, -1, &fontBtn, rc, &sfCenter, &btnText);
    };

    int state = layout.copiedState;
    if (state != 0) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - layout.copiedTime).count() > 2000) {
            state = 0;
        }
    }

    DrawBtn(layout.rcCopyQuestion, state == 1 ? L"已复制" : L"复制题目", layout.isCopyQHovered, state == 1);
    DrawBtn(layout.rcCopyAll, state == 2 ? L"已复制" : L"复制全题", layout.isCopyAllHovered, state == 2);

    // 3. Question Text
    float innerY = headerY + (float)Scale(28, dpi) + (float)Scale(10, dpi);
    float contentW = layout.rcCard.Width - (padH * 2.0f);
    Font fontQuestion(L"Microsoft YaHei UI", (float)Scale(13, dpi), FontStyleRegular, UnitPixel);
    SolidBrush brushQText(theme.textPrimary);
    StringFormat sfWrap(StringFormat::GenericTypographic());

    RectF rcQ(layout.rcCard.X + padH, innerY, contentW, 10000.0f);
    RectF boundQ;
    g.MeasureString(q.issue.c_str(), -1, &fontQuestion, rcQ, &sfWrap, &boundQ);
    g.DrawString(q.issue.c_str(), -1, &fontQuestion, RectF(rcQ.X, rcQ.Y, contentW, boundQ.Height + (float)Scale(2, dpi)), &sfWrap, &brushQText);

    innerY += boundQ.Height + (float)Scale(12, dpi);

    // 4. Options Container (if options exist)
    if (!q.options.empty()) {
        float optInnerPadV = (float)Scale(10, dpi);
        float optInnerPadH = (float)Scale(14, dpi);
        float optContentW = contentW - (optInnerPadH * 2.0f);
        float optBoxX = layout.rcCard.X + padH;
        float optBoxY = innerY;

        Font fontOption(L"Microsoft YaHei UI", (float)Scale(11, dpi), FontStyleRegular, UnitPixel);
        float optTotalH = optInnerPadV * 2.0f;
        std::vector<RectF> optBounds;
        for (const auto& opt : q.options) {
            RectF layoutOpt(0, 0, optContentW, 10000.0f);
            RectF boundOpt;
            g.MeasureString(opt.c_str(), -1, &fontOption, layoutOpt, &sfWrap, &boundOpt);
            optBounds.push_back(boundOpt);
            optTotalH += boundOpt.Height + (float)Scale(6, dpi);
        }

        // Draw Options Box Background
        RectF rcOptBox(optBoxX, optBoxY, contentW, optTotalH);
        SolidBrush brushOptBg(theme.bgOptions);
        Pen penOptBox(theme.borderOptions, 1.0f);
        DrawRoundedRect(g, &brushOptBg, &penOptBox, rcOptBox, (float)Scale(6, dpi));

        // Draw Each Option Text
        SolidBrush brushOptText(theme.textOptions);
        SolidBrush brushCorrectOptText(theme.badgeJudgeText);
        SolidBrush brushCorrectPillBg(theme.badgeJudgeBg);
        Pen penCorrectPill(theme.badgeJudgeBorder, 1.0f);
        Font fontOptBold(L"Microsoft YaHei UI", (float)Scale(11, dpi), FontStyleBold, UnitPixel);

        float itemY = optBoxY + optInnerPadV;
        for (size_t i = 0; i < q.options.size(); ++i) {
            const std::wstring& optStr = q.options[i];
            float lineH = optBounds[i].Height;

            // Check if this option is one of the correct answers
            bool isCorrectOpt = false;
            if (!optStr.empty()) {
                wchar_t optPrefix = optStr[0];
                if (optPrefix >= L'A' && optPrefix <= L'Z') {
                    if (q.answer.find(optPrefix) != std::wstring::npos) {
                        isCorrectOpt = true;
                    }
                }
            }

            RectF rcItem(optBoxX + optInnerPadH, itemY, optContentW, lineH + (float)Scale(2, dpi));

            if (isCorrectOpt) {
                // Highlight correct option line with subtle rounded rect
                RectF rcLineBg(optBoxX + (float)Scale(4, dpi), itemY - (float)Scale(2, dpi), 
                               contentW - (float)Scale(8, dpi), lineH + (float)Scale(6, dpi));
                DrawRoundedRect(g, &brushCorrectPillBg, &penCorrectPill, rcLineBg, (float)Scale(4, dpi));
                g.DrawString(optStr.c_str(), -1, &fontOptBold, rcItem, &sfWrap, &brushCorrectOptText);
            } else {
                g.DrawString(optStr.c_str(), -1, &fontOption, rcItem, &sfWrap, &brushOptText);
            }

            itemY += lineH + (float)Scale(6, dpi);
        }

        innerY += optTotalH + (float)Scale(12, dpi);
    }

    // 5. Answer Container
    float answerH = (float)Scale(36, dpi);
    RectF rcAnswer(layout.rcCard.X + padH, innerY, contentW, answerH);

    bool isJudge = (q.type == L"判断题");
    bool isTrue = (q.answer == L"对" || q.answer == L"正确");

    Color ansBg = isJudge ? (isTrue ? theme.badgeJudgeBg : Color(254, 242, 242)) : theme.badgeSingleBg;
    Color ansBorder = isJudge ? (isTrue ? theme.badgeJudgeBorder : Color(254, 202, 202)) : theme.badgeSingleBorder;
    Color ansText = isJudge ? (isTrue ? theme.badgeJudgeText : Color(220, 38, 38)) : theme.badgeSingleText;

    if (theme.isDark && isJudge && !isTrue) {
        ansBg = Color(69, 10, 10);
        ansBorder = Color(153, 27, 27);
        ansText = Color(248, 113, 113);
    }

    SolidBrush brushAnswerBg(ansBg);
    Pen penAnswer(ansBorder, 1.0f);
    DrawRoundedRect(g, &brushAnswerBg, &penAnswer, rcAnswer, (float)Scale(6, dpi));

    std::wstring answerText;
    if (isJudge) {
        answerText = isTrue ? L"正确答案：【 对 / 正确 】" : L"正确答案：【 错 / 错误 】";
    } else {
        answerText = L"正确答案：" + (q.answer.empty() ? L"未知" : q.answer);
    }

    SolidBrush brushAnswerText(ansText);
    Font fontAnswer(L"Microsoft YaHei UI", (float)Scale(12, dpi), FontStyleBold, UnitPixel);
    
    RectF rcAnswerText(rcAnswer.X + (float)Scale(14, dpi), rcAnswer.Y, contentW - (float)Scale(28, dpi), answerH);
    g.DrawString(answerText.c_str(), -1, &fontAnswer, rcAnswerText, &sfLeftVCenter, &brushAnswerText);
}
