#include "gfx.h"
#include <d2d1helper.h>

namespace {

// Local IID copies: MinGW's libuuid does not ship Direct2D/DirectWrite IIDs, and
// linking against uuid.lib is not an option for a fully static portable build.
const IID kIID_ID2D1Factory = { 0x06152247, 0x6f50, 0x465a, { 0x92, 0x45, 0x11, 0x8b, 0xfd, 0x3b, 0x60, 0x07 } };

D2D1_MATRIX_3X2_F Identity() {
    D2D1_MATRIX_3X2_F m = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    return m;
}

D2D1_RECT_F ToRect(const RectF& rc) { return rc.Raw(); }

D2D1_ROUNDED_RECT ToRoundRect(const RectF& rc, float radius) {
    D2D1_ROUNDED_RECT rr;
    rr.rect = ToRect(rc);
    rr.radiusX = radius;
    rr.radiusY = radius;
    return rr;
}

D2D1_POINT_2F P(float x, float y) { D2D1_POINT_2F p = { x, y }; return p; }

}  // namespace

bool Gfx::Init() {
    D2D1_FACTORY_OPTIONS options = {};
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, kIID_ID2D1Factory,
                                   &options, reinterpret_cast<void**>(&m_factory));
    if (FAILED(hr) || !m_factory) return false;

    D2D1_STROKE_STYLE_PROPERTIES strokeProps = {};
    strokeProps.startCap = D2D1_CAP_STYLE_ROUND;
    strokeProps.endCap = D2D1_CAP_STYLE_ROUND;
    strokeProps.dashCap = D2D1_CAP_STYLE_ROUND;
    strokeProps.lineJoin = D2D1_LINE_JOIN_ROUND;
    strokeProps.miterLimit = 2.0f;
    strokeProps.dashStyle = D2D1_DASH_STYLE_SOLID;
    strokeProps.dashOffset = 0.0f;
    m_factory->CreateStrokeStyle(strokeProps, nullptr, 0, &m_roundStroke);
    return true;
}

void Gfx::Shutdown() {
    DiscardTarget();
    ReleaseBrushes();
    if (m_layer) { m_layer->Release(); m_layer = nullptr; }
    if (m_clipGeometry) { m_clipGeometry->Release(); m_clipGeometry = nullptr; }
    if (m_roundStroke) { m_roundStroke->Release(); m_roundStroke = nullptr; }
    if (m_factory) { m_factory->Release(); m_factory = nullptr; }
}

HRESULT Gfx::EnsureTarget(HWND hwnd, UINT widthPx, UINT heightPx, float dpi) {
    if (!m_factory || widthPx == 0 || heightPx == 0) return E_FAIL;

    if (!m_rt) {
        D2D1_RENDER_TARGET_PROPERTIES props = {};
        props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        props.dpiX = dpi;
        props.dpiY = dpi;
        props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

        D2D1_SIZE_U pixelSize = { widthPx, heightPx };
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = {};
        hwndProps.hwnd = hwnd;
        hwndProps.pixelSize = pixelSize;
        hwndProps.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

        HRESULT hr = m_factory->CreateHwndRenderTarget(props, hwndProps, &m_rt);
        if (FAILED(hr) || !m_rt) {
            m_rt = nullptr;
            return hr;
        }
        m_rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        m_rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        m_rt->SetDpi(dpi, dpi);
        return S_OK;
    }

    D2D1_SIZE_U pixelSize = { widthPx, heightPx };
    HRESULT hr = m_rt->Resize(pixelSize);
    if (FAILED(hr)) {
        DiscardTarget();
        return EnsureTarget(hwnd, widthPx, heightPx, dpi);
    }
    m_rt->SetDpi(dpi, dpi);
    return S_OK;
}

void Gfx::DiscardTarget() {
    ReleaseBrushes();
    if (m_layer) { m_layer->Release(); m_layer = nullptr; }
    if (m_clipGeometry) { m_clipGeometry->Release(); m_clipGeometry = nullptr; }
    m_clipRect = RectF();
    m_clipRadius = 0.0f;
    m_roundClipDepth = 0;
    if (m_rt) {
        m_rt->Release();
        m_rt = nullptr;
    }
    m_clipDepth = 0;
}

void Gfx::ReleaseBrushes() {
    for (auto& entry : m_brushes) {
        if (entry.second) entry.second->Release();
    }
    m_brushes.clear();
}

HRESULT Gfx::BeginDraw() {
    if (!m_rt) return E_FAIL;
    m_clipDepth = 0;
    m_rt->BeginDraw();
    m_rt->SetTransform(Identity());
    return S_OK;
}

HRESULT Gfx::EndDraw() {
    if (!m_rt) return E_FAIL;
    while (m_clipDepth > 0) {
        m_rt->PopAxisAlignedClip();
        --m_clipDepth;
    }
    HRESULT hr = m_rt->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardTarget();
    }
    return hr;
}

ID2D1SolidColorBrush* Gfx::Brush(Color color) {
    if (!m_rt) return nullptr;
    UINT32 key = PackColorKey(color);
    auto it = m_brushes.find(key);
    if (it != m_brushes.end()) return it->second;
    ID2D1SolidColorBrush* brush = nullptr;
    if (FAILED(m_rt->CreateSolidColorBrush(color, &brush)) || !brush) return nullptr;
    m_brushes[key] = brush;
    return brush;
}

void Gfx::Clear(Color color) {
    if (m_rt) m_rt->Clear(color);
}

void Gfx::FillRect(const RectF& rc, Color color) {
    ID2D1SolidColorBrush* b = Brush(color);
    if (m_rt && b) m_rt->FillRectangle(ToRect(rc), b);
}

void Gfx::FillRound(const RectF& rc, float radius, Color color) {
    ID2D1SolidColorBrush* b = Brush(color);
    if (!m_rt || !b) return;
    if (radius <= 0.0f) {
        m_rt->FillRectangle(ToRect(rc), b);
        return;
    }
    m_rt->FillRoundedRectangle(ToRoundRect(rc, radius), b);
}

void Gfx::StrokeRound(const RectF& rc, float radius, Color color, float strokeWidth) {
    ID2D1SolidColorBrush* b = Brush(color);
    if (!m_rt || !b) return;
    if (radius <= 0.0f) {
        m_rt->DrawRectangle(ToRect(rc), b, strokeWidth);
        return;
    }
    m_rt->DrawRoundedRectangle(ToRoundRect(rc, radius), b, strokeWidth);
}

void Gfx::FillCircle(float cx, float cy, float radius, Color color) {
    ID2D1SolidColorBrush* b = Brush(color);
    if (!m_rt || !b) return;
    D2D1_ELLIPSE ellipse;
    ellipse.point = P(cx, cy);
    ellipse.radiusX = radius;
    ellipse.radiusY = radius;
    m_rt->FillEllipse(ellipse, b);
}

void Gfx::StrokeCircle(float cx, float cy, float radius, Color color, float strokeWidth) {
    ID2D1SolidColorBrush* b = Brush(color);
    if (!m_rt || !b) return;
    D2D1_ELLIPSE ellipse;
    ellipse.point = P(cx, cy);
    ellipse.radiusX = radius;
    ellipse.radiusY = radius;
    m_rt->DrawEllipse(ellipse, b, strokeWidth);
}

void Gfx::Line(float x1, float y1, float x2, float y2, Color color, float strokeWidth) {
    ID2D1SolidColorBrush* b = Brush(color);
    if (!m_rt || !b) return;
    m_rt->DrawLine(P(x1, y1), P(x2, y2), b, strokeWidth, m_roundStroke);
}

void Gfx::DrawTextLayout(IDWriteTextLayout* layout, float x, float y, Color color) {
    if (!m_rt || !layout) return;
    ID2D1SolidColorBrush* b = Brush(color);
    if (!b) return;
    m_rt->DrawTextLayout(P(x, y), layout, b, D2D1_DRAW_TEXT_OPTIONS_NONE);
}

void Gfx::PushClip(const RectF& rc) {
    if (!m_rt) return;
    m_rt->PushAxisAlignedClip(ToRect(rc), D2D1_ANTIALIAS_MODE_ALIASED);
    ++m_clipDepth;
}

void Gfx::PopClip() {
    if (!m_rt || m_clipDepth <= 0) return;
    m_rt->PopAxisAlignedClip();
    --m_clipDepth;
}

void Gfx::PushRoundClip(const RectF& rc, float radius) {
    if (!m_rt || !m_factory) return;
    if (!m_layer && FAILED(m_rt->CreateLayer(&m_layer))) return;

    if (!m_clipGeometry || std::fabs(m_clipRect.x - rc.x) > 0.01f ||
        std::fabs(m_clipRect.y - rc.y) > 0.01f || std::fabs(m_clipRect.w - rc.w) > 0.01f ||
        std::fabs(m_clipRect.h - rc.h) > 0.01f || std::fabs(m_clipRadius - radius) > 0.01f) {
        if (m_clipGeometry) { m_clipGeometry->Release(); m_clipGeometry = nullptr; }
        D2D1_ROUNDED_RECT rr = ToRoundRect(rc, radius);
        if (FAILED(m_factory->CreateRoundedRectangleGeometry(rr, &m_clipGeometry)) || !m_clipGeometry) {
            return;
        }
        m_clipRect = rc;
        m_clipRadius = radius;
    }

    D2D1_LAYER_PARAMETERS params = {};
    params.contentBounds = ToRect(rc);
    params.geometricMask = m_clipGeometry;
    params.maskAntialiasMode = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
    params.maskTransform = Identity();
    params.opacity = 1.0f;
    params.layerOptions = D2D1_LAYER_OPTIONS_NONE;
    m_rt->PushLayer(params, m_layer);
    ++m_roundClipDepth;
}

void Gfx::PopRoundClip() {
    if (!m_rt || m_roundClipDepth <= 0) return;
    m_rt->PopLayer();
    --m_roundClipDepth;
}

void Gfx::ShadowCircle(float cx, float cy, float radius, Color color) {
    if (!m_rt) return;
    for (int i = 3; i >= 1; --i) {
        Color c = color;
        c.a = color.a * (0.42f / (float)i);
        FillCircle(cx, cy, radius + (float)i * 1.5f, c);
    }
}

void Gfx::DrawIcon(Icon icon, const RectF& box, Color color, float strokeWidth) {
    if (!m_rt || !m_factory) return;
    const float x = box.x;
    const float y = box.y;
    const float w = box.w;
    const float h = box.h;
    auto px = [&](float nx) { return x + nx * w; };
    auto py = [&](float ny) { return y + ny * h; };

    ID2D1SolidColorBrush* brush = Brush(color);
    if (!brush) return;

    switch (icon) {
    case Icon::Search: {
        m_rt->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(px(0.42f), py(0.42f)), w * 0.27f, h * 0.27f),
                          brush, strokeWidth, m_roundStroke);
        m_rt->DrawLine(D2D1::Point2F(px(0.63f), py(0.63f)), D2D1::Point2F(px(0.85f), py(0.85f)),
                       brush, strokeWidth, m_roundStroke);
        break;
    }
    case Icon::Close: {
        m_rt->DrawLine(D2D1::Point2F(px(0.27f), py(0.27f)), D2D1::Point2F(px(0.73f), py(0.73f)),
                       brush, strokeWidth, m_roundStroke);
        m_rt->DrawLine(D2D1::Point2F(px(0.73f), py(0.27f)), D2D1::Point2F(px(0.27f), py(0.73f)),
                       brush, strokeWidth, m_roundStroke);
        break;
    }
    case Icon::Cross: {
        m_rt->DrawLine(D2D1::Point2F(px(0.24f), py(0.24f)), D2D1::Point2F(px(0.76f), py(0.76f)),
                       brush, strokeWidth, m_roundStroke);
        m_rt->DrawLine(D2D1::Point2F(px(0.76f), py(0.24f)), D2D1::Point2F(px(0.24f), py(0.76f)),
                       brush, strokeWidth, m_roundStroke);
        break;
    }
    case Icon::Check: {
        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(m_factory->CreatePathGeometry(&path)) && path) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink)) && sink) {
                sink->BeginFigure(D2D1::Point2F(px(0.20f), py(0.53f)), D2D1_FIGURE_BEGIN_HOLLOW);
                sink->AddLine(D2D1::Point2F(px(0.42f), py(0.75f)));
                sink->AddLine(D2D1::Point2F(px(0.80f), py(0.26f)));
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->Close();
                sink->Release();
                m_rt->DrawGeometry(path, brush, strokeWidth, m_roundStroke);
            }
            path->Release();
        }
        break;
    }
    case Icon::ArrowUp: {
        m_rt->DrawLine(D2D1::Point2F(px(0.5f), py(0.78f)), D2D1::Point2F(px(0.5f), py(0.24f)),
                       brush, strokeWidth, m_roundStroke);
        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(m_factory->CreatePathGeometry(&path)) && path) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink)) && sink) {
                sink->BeginFigure(D2D1::Point2F(px(0.27f), py(0.47f)), D2D1_FIGURE_BEGIN_HOLLOW);
                sink->AddLine(D2D1::Point2F(px(0.5f), py(0.25f)));
                sink->AddLine(D2D1::Point2F(px(0.73f), py(0.47f)));
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->Close();
                sink->Release();
                m_rt->DrawGeometry(path, brush, strokeWidth, m_roundStroke);
            }
            path->Release();
        }
        break;
    }
    case Icon::Copy: {
        // Front sheet
        RectF front(px(0.34f), py(0.34f), w * 0.46f, h * 0.46f);
        m_rt->DrawRoundedRectangle(ToRoundRect(front, w * 0.10f), brush, strokeWidth);
        // Back sheet (open corner)
        ID2D1PathGeometry* path = nullptr;
        if (SUCCEEDED(m_factory->CreatePathGeometry(&path)) && path) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(path->Open(&sink)) && sink) {
                sink->BeginFigure(D2D1::Point2F(px(0.20f), py(0.66f)), D2D1_FIGURE_BEGIN_HOLLOW);
                sink->AddLine(D2D1::Point2F(px(0.20f), py(0.24f)));
                sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(D2D1::Point2F(px(0.20f), py(0.20f)),
                                                                     D2D1::Point2F(px(0.24f), py(0.20f))));
                sink->AddLine(D2D1::Point2F(px(0.66f), py(0.20f)));
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->Close();
                sink->Release();
                m_rt->DrawGeometry(path, brush, strokeWidth, m_roundStroke);
            }
            path->Release();
        }
        break;
    }
    case Icon::Sun: {
        m_rt->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(px(0.5f), py(0.5f)), w * 0.19f, h * 0.19f),
                          brush, strokeWidth, m_roundStroke);
        for (int i = 0; i < 8; ++i) {
            float angle = (float)i * 3.14159265f / 4.0f;
            float c = cosf(angle);
            float s = sinf(angle);
            m_rt->DrawLine(D2D1::Point2F(px(0.5f + 0.31f * c), py(0.5f + 0.31f * s)),
                           D2D1::Point2F(px(0.5f + 0.44f * c), py(0.5f + 0.44f * s)),
                           brush, strokeWidth, m_roundStroke);
        }
        break;
    }
    case Icon::Moon: {
        // Crescent = big disc EXCLUDE offset disc.
        ID2D1EllipseGeometry* outer = nullptr;
        ID2D1EllipseGeometry* inner = nullptr;
        m_factory->CreateEllipseGeometry(
            D2D1::Ellipse(D2D1::Point2F(px(0.46f), py(0.52f)), w * 0.34f, h * 0.34f), &outer);
        m_factory->CreateEllipseGeometry(
            D2D1::Ellipse(D2D1::Point2F(px(0.68f), py(0.38f)), w * 0.30f, h * 0.30f), &inner);
        if (outer && inner) {
            ID2D1PathGeometry* crescent = nullptr;
            if (SUCCEEDED(m_factory->CreatePathGeometry(&crescent)) && crescent) {
                ID2D1GeometrySink* sink = nullptr;
                if (SUCCEEDED(crescent->Open(&sink)) && sink) {
                    outer->CombineWithGeometry(inner, D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink);
                    sink->Close();
                    sink->Release();
                    m_rt->FillGeometry(crescent, brush);
                }
                crescent->Release();
            }
        }
        if (outer) outer->Release();
        if (inner) inner->Release();
        break;
    }
    default:
        break;
    }
}