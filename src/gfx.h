#pragma once
#include "common.h"

enum class Icon {
    Search,
    Close,
    Sun,
    Moon,
    Copy,
    Check,
    Cross,
    ArrowUp
};

// Thin Direct2D wrapper: device target lifecycle, cached brushes, and the few
// primitives the whole UI is built from.
class Gfx {
public:
    bool Init();
    void Shutdown();

    HRESULT EnsureTarget(HWND hwnd, UINT widthPx, UINT heightPx, float dpi);
    void DiscardTarget();

    ID2D1HwndRenderTarget* RT() const { return m_rt; }
    ID2D1Factory* Factory() const { return m_factory; }

    HRESULT BeginDraw();
    HRESULT EndDraw();

    ID2D1SolidColorBrush* Brush(Color color);

    void Clear(Color color);
    void FillRect(const RectF& rc, Color color);
    void FillRound(const RectF& rc, float radius, Color color);
    void StrokeRound(const RectF& rc, float radius, Color color, float strokeWidth = 1.0f);
    void FillCircle(float cx, float cy, float radius, Color color);
    void StrokeCircle(float cx, float cy, float radius, Color color, float strokeWidth = 1.0f);
    void Line(float x1, float y1, float x2, float y2, Color color, float strokeWidth = 1.0f);
    void DrawIcon(Icon icon, const RectF& box, Color color, float strokeWidth = 1.6f);
    void DrawTextLayout(IDWriteTextLayout* layout, float x, float y, Color color);

    void PushClip(const RectF& rc);
    void PopClip();
    // Rounded clip used to keep full-bleed row hovers inside the panel corners.
    void PushRoundClip(const RectF& rc, float radius);
    void PopRoundClip();

    // Soft drop shadow used by floating chrome (drawn as a few translucent rings).
    void ShadowCircle(float cx, float cy, float radius, Color color);

private:
    void ReleaseBrushes();

    ID2D1Factory* m_factory = nullptr;
    ID2D1HwndRenderTarget* m_rt = nullptr;
    ID2D1StrokeStyle* m_roundStroke = nullptr;
    std::unordered_map<UINT32, ID2D1SolidColorBrush*> m_brushes;
    ID2D1Layer* m_layer = nullptr;
    ID2D1RoundedRectangleGeometry* m_clipGeometry = nullptr;
    RectF m_clipRect;
    float m_clipRadius = 0.0f;
    int m_clipDepth = 0;
    int m_roundClipDepth = 0;
};