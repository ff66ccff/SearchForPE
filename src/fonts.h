#pragma once
#include "common.h"

// Semantic text roles. Every role owns its size (DIP), weight and line box so
// painting code never hardcodes typography numbers.
enum class TextRole {
    Question,     // 题干 15 / 1.7
    Option,       // 选项 13.5 / 1.6
    OptionStrong, // 正确选项
    Meta,         // 序号 / 类型 / 计数 11.5
    MetaStrong,   // 序号 / 计数强调
    MetaMedium,   // 关键词 chip / 计数数字
    StatusStrong,
    Pill,         // 答案胶囊 12
    Filter,       // 筛选胶囊 12.5
    FilterStrong,
    Status,       // 结果统计行 12
    EmptyTitle,
    EmptyHint,
    Toast,
    Field,        // 搜索框文本 / 占位符
    Count
};

class FontManager {
public:
    bool Init();
    void Shutdown();

    IDWriteFactory* Dw() const { return m_dw; }
    IDWriteTextFormat* Format(TextRole role) const { return m_formats[(int)role]; }

    IDWriteTextLayout* MakeLayout(const std::wstring& text, TextRole role, float maxWidth,
                                  bool wrap = true) const;
    float MeasureHeight(const std::wstring& text, TextRole role, float maxWidth) const;
    float MeasureWidth(const std::wstring& text, TextRole role) const;

    // Native single-line edit font (GDI, for the IME-backed search field).
    HFONT CreateGdiFont(TextRole role, UINT dpi) const;

    const std::wstring& Family() const { return m_family; }
    const std::wstring& StrongFamily() const { return m_family; }
    const std::wstring& Description() const { return m_description; }

private:
    struct RoleSpec {
        float size;
        int weight;      // DWRITE_FONT_WEIGHT
        float lineHeight;
    };
    static RoleSpec SpecFor(TextRole role);

    IDWriteFactory* m_dw = nullptr;
    IDWriteTextFormat* m_formats[(int)TextRole::Count] = {};
    IDWriteTextFormat* m_formatsNoWrap[(int)TextRole::Count] = {};
    std::wstring m_family;
    std::wstring m_description;
};