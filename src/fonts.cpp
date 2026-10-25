#include "fonts.h"

namespace {

const IID kIID_IDWriteFactory = { 0xb859ee5a, 0xd838, 0x4b5b,
                                   { 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48 } };

bool FamilyExists(IDWriteFontCollection* collection, const wchar_t* name) {
    if (!collection || !name) return false;
    UINT32 index = 0;
    BOOL exists = FALSE;
    collection->FindFamilyName(name, &index, &exists);
    return exists == TRUE;
}

std::wstring FirstExisting(IDWriteFontCollection* collection, const wchar_t* const* names, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (FamilyExists(collection, names[i])) return names[i];
    }
    return L"Microsoft YaHei UI";
}

// MiSans VF is a variable font: Windows exposes real instances at 150..700.
// Weights below are deliberately "off the enum" so DirectWrite matches the
// actual variable instances instead of rounding to Regular/Medium/Bold.
constexpr int kWeightRead = 400;     // body copy (variable instance 450 is closest real face)
constexpr int kWeightMedium = 520;   // titles, empty state
constexpr int kWeightStrong = 630;   // labels, pills, selected filters

}  // namespace

FontManager::RoleSpec FontManager::SpecFor(TextRole role) {
    switch (role) {
    case TextRole::Question:     return { 15.0f, kWeightRead,   25.5f };
    case TextRole::Option:       return { 13.5f, kWeightRead,   21.6f };
    case TextRole::OptionStrong: return { 13.5f, kWeightStrong, 21.6f };
    case TextRole::Meta:         return { 11.5f, kWeightRead,   16.0f };
    case TextRole::MetaStrong:   return { 11.5f, kWeightStrong, 16.0f };
    case TextRole::MetaMedium:   return { 11.5f, kWeightMedium, 16.0f };
    case TextRole::StatusStrong: return { 12.0f, kWeightMedium,   16.0f };
    case TextRole::Pill:         return { 12.0f, kWeightStrong, 16.0f };
    case TextRole::Filter:       return { 12.5f, kWeightRead,   18.0f };
    case TextRole::FilterStrong: return { 12.5f, kWeightStrong, 18.0f };
    case TextRole::Status:       return { 12.0f, kWeightRead,   16.0f };
    case TextRole::EmptyTitle:   return { 14.0f, kWeightMedium, 22.0f };
    case TextRole::EmptyHint:    return { 12.5f, kWeightRead,   19.0f };
    case TextRole::Toast:        return { 12.0f, kWeightMedium, 16.0f };
    case TextRole::Field:        return { 15.0f, kWeightRead,   20.0f };
    default:                     return { 12.0f, kWeightRead,   16.0f };
    }
}

bool FontManager::Init() {
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, kIID_IDWriteFactory,
                                   reinterpret_cast<IUnknown**>(&m_dw))) || !m_dw) {
        return false;
    }

    IDWriteFontCollection* collection = nullptr;
    m_dw->GetSystemFontCollection(&collection, FALSE);

    static const wchar_t* kFamilies[] = {
        L"MiSans VF", L"MiSans", L"Noto Sans SC", L"HarmonyOS Sans SC",
        L"Microsoft YaHei UI", L"Microsoft YaHei", L"Segoe UI"
    };
    m_family = FirstExisting(collection, kFamilies, sizeof(kFamilies) / sizeof(kFamilies[0]));
    m_description = m_family;
    if (collection) collection->Release();

    for (int i = 0; i < (int)TextRole::Count; ++i) {
        RoleSpec spec = SpecFor((TextRole)i);
        HRESULT hr = m_dw->CreateTextFormat(m_family.c_str(), nullptr, (DWRITE_FONT_WEIGHT)spec.weight,
                                            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                            spec.size, L"zh-CN", &m_formats[i]);
        if (FAILED(hr)) return false;
        m_formats[i]->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        m_formats[i]->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, spec.lineHeight,
                                     spec.lineHeight * 0.78f);
        m_formats[i]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_formats[i]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        hr = m_dw->CreateTextFormat(m_family.c_str(), nullptr, (DWRITE_FONT_WEIGHT)spec.weight,
                                    DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, spec.size,
                                    L"zh-CN", &m_formatsNoWrap[i]);
        if (FAILED(hr)) return false;
        m_formatsNoWrap[i]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_formatsNoWrap[i]->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, spec.lineHeight,
                                           spec.lineHeight * 0.78f);
        m_formatsNoWrap[i]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_formatsNoWrap[i]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
    return true;
}

void FontManager::Shutdown() {
    for (int i = 0; i < (int)TextRole::Count; ++i) {
        if (m_formats[i]) { m_formats[i]->Release(); m_formats[i] = nullptr; }
        if (m_formatsNoWrap[i]) { m_formatsNoWrap[i]->Release(); m_formatsNoWrap[i] = nullptr; }
    }
    if (m_dw) { m_dw->Release(); m_dw = nullptr; }
}

IDWriteTextLayout* FontManager::MakeLayout(const std::wstring& text, TextRole role, float maxWidth,
                                           bool wrap) const {
    if (!m_dw || text.empty() || maxWidth <= 1.0f) return nullptr;
    IDWriteTextFormat* format = wrap ? m_formats[(int)role] : m_formatsNoWrap[(int)role];
    if (!format) return nullptr;
    IDWriteTextLayout* layout = nullptr;
    HRESULT hr = m_dw->CreateTextLayout(text.c_str(), (UINT32)text.size(), format, maxWidth, 4096.0f,
                                        &layout);
    if (FAILED(hr)) return nullptr;
    return layout;
}

float FontManager::MeasureHeight(const std::wstring& text, TextRole role, float maxWidth) const {
    IDWriteTextLayout* layout = MakeLayout(text, role, maxWidth);
    if (!layout) return 0.0f;
    DWRITE_TEXT_METRICS metrics = {};
    layout->GetMetrics(&metrics);
    float height = metrics.height;
    layout->Release();
    return height;
}

float FontManager::MeasureWidth(const std::wstring& text, TextRole role) const {
    IDWriteTextLayout* layout = MakeLayout(text, role, 4096.0f, false);
    if (!layout) return 0.0f;
    DWRITE_TEXT_METRICS metrics = {};
    layout->GetMetrics(&metrics);
    float width = metrics.widthIncludingTrailingWhitespace;
    layout->Release();
    return width;
}

HFONT FontManager::CreateGdiFont(TextRole role, UINT dpi) const {
    RoleSpec spec = SpecFor(role);
    int height = -MulDiv((int)std::lround(spec.size * 1.00f), (int)dpi, 96);

    // GDI does not merge variable-font instances into one family the way
    // DirectWrite does, so "MiSans VF" may resolve to an arbitrary (often
    // heavier) instance. Probe explicit face names and keep the first one GDI
    // actually honours, so the native edit matches the DWrite text next to it.
    std::vector<std::wstring> candidates;
    if (spec.weight >= 600) {
        candidates.push_back(m_family + L" Semibold");
        candidates.push_back(m_family + L" Demibold");
    } else {
        candidates.push_back(m_family + L" Regular");
        candidates.push_back(m_family + L" Normal");
    }
    candidates.push_back(m_family);
    candidates.push_back(L"Microsoft YaHei UI");

    HDC screen = GetDC(nullptr);
    for (const auto& face : candidates) {
        LOGFONTW lf = {};
        lf.lfHeight = height;
        lf.lfWeight = spec.weight >= 600 ? FW_SEMIBOLD : FW_NORMAL;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = CLEARTYPE_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        wcsncpy(lf.lfFaceName, face.c_str(), LF_FACESIZE - 1);
        HFONT font = CreateFontIndirectW(&lf);
        if (!font) continue;
        if (!screen) return font;
        HGDIOBJ old = SelectObject(screen, font);
        wchar_t resolved[LF_FACESIZE] = {};
        GetTextFaceW(screen, LF_FACESIZE, resolved);
        SelectObject(screen, old);
        if (_wcsicmp(resolved, face.c_str()) == 0) {
            ReleaseDC(nullptr, screen);
            return font;
        }
        DeleteObject(font);
    }
    if (screen) ReleaseDC(nullptr, screen);

    LOGFONTW lf = {};
    lf.lfHeight = height;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcsncpy(lf.lfFaceName, L"Microsoft YaHei UI", LF_FACESIZE - 1);
    return CreateFontIndirectW(&lf);
}