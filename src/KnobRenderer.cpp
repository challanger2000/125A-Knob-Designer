#include "KnobRenderer.h"

#include <windows.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace knob125a {
namespace {

using namespace Gdiplus;

constexpr float kPi = 3.14159265358979323846f;

Gdiplus::Color gdipColor(const knob125a::Color& c) {
    return Gdiplus::Color(c.a, c.r, c.g, c.b);
}

int getEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    if (GetImageEncodersSize(&num, &size) != Ok || size == 0) {
        return -1;
    }

    std::vector<BYTE> buffer(size);
    auto* codecs = reinterpret_cast<ImageCodecInfo*>(buffer.data());
    if (GetImageEncoders(num, size, codecs) != Ok) {
        return -1;
    }

    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, format) == 0) {
            *pClsid = codecs[i].Clsid;
            return static_cast<int>(i);
        }
    }
    return -1;
}

RectF centeredCircle(float cx, float cy, float radius) {
    return RectF(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
}

void drawFrame(
    Graphics& g,
    const KnobStyle& style,
    int xOffset,
    int yOffset,
    int cellSize,
    float angleDeg) {

    const float size = static_cast<float>(cellSize);
    const float cx = static_cast<float>(xOffset) + size * 0.5f;
    const float cy = static_cast<float>(yOffset) + size * 0.5f;
    const float half = size * 0.5f;

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    {
        const float r = half * style.bezelRadius * style.shadowScale;
        SolidBrush brush(gdipColor(style.shadow));
        g.FillEllipse(&brush, centeredCircle(cx, cy + style.shadowOffsetY, r));
    }

    {
        const float r = half * style.bezelRadius;
        LinearGradientBrush brush(
            PointF(cx, cy - r),
            PointF(cx, cy + r),
            gdipColor(style.bezelOuter),
            gdipColor(style.bezelInner));
        g.FillEllipse(&brush, centeredCircle(cx, cy, r));

        Pen edgePen(gdipColor(style.edge), std::max(1.0f, size / 128.0f));
        g.DrawEllipse(&edgePen, centeredCircle(cx, cy, r));
    }

    {
        const float r = half * style.bodyRadius;
        LinearGradientBrush brush(
            PointF(cx - r * 0.4f, cy - r),
            PointF(cx + r * 0.3f, cy + r),
            gdipColor(style.bodyTop),
            gdipColor(style.bodyBottom));
        g.FillEllipse(&brush, centeredCircle(cx, cy, r));

        Pen hiPen(gdipColor(style.highlight), std::max(1.0f, size * 0.018f));
        const RectF hiRect = centeredCircle(cx, cy, r * 0.90f);
        g.DrawArc(&hiPen, hiRect, 205.0f, 112.0f);
    }

    if (style.drawCenterCap) {
        const float capR = half * 0.105f;
        SolidBrush cap(gdipColor(style.centerCap));
        g.FillEllipse(&cap, centeredCircle(cx, cy, capR));
    }

    {
        const float rad = (angleDeg - 90.0f) * kPi / 180.0f;
        const float r1 = half * style.indicatorInnerRadius;
        const float r2 = half * style.indicatorOuterRadius;

        const PointF p1(
            cx + std::cos(rad) * r1,
            cy + std::sin(rad) * r1);
        const PointF p2(
            cx + std::cos(rad) * r2,
            cy + std::sin(rad) * r2);

        Pen indicatorPen(
            gdipColor(style.indicator),
            std::max(1.0f, style.indicatorWidth * size / 128.0f));
        indicatorPen.SetStartCap(LineCapRound);
        indicatorPen.SetEndCap(LineCapRound);
        g.DrawLine(&indicatorPen, p1, p2);
    }
}

class GdiPlusSession {
public:
    GdiPlusSession() {
        GdiplusStartupInput input;
        if (GdiplusStartup(&token_, &input, nullptr) != Ok) {
            token_ = 0;
        }
    }

    ~GdiPlusSession() {
        if (token_ != 0) {
            GdiplusShutdown(token_);
        }
    }

    bool ok() const { return token_ != 0; }

private:
    ULONG_PTR token_ {0};
};

} // namespace

std::vector<KnobStyle> KnobRenderer::builtInStyles() {
    return {
        {
            L"Black Studio",
            {92, 0, 0, 0},
            {255, 44, 46, 49},
            {255, 13, 14, 16},
            {255, 56, 59, 63},
            {255, 15, 17, 20},
            {255, 7, 8, 10},
            {110, 220, 225, 230},
            {255, 238, 241, 244},
            {255, 28, 30, 33},
            4.0f, 1.02f, 0.44f, 0.385f, 0.18f, 0.32f, 3.0f, false
        },
        {
            L"Gunmetal",
            {96, 0, 0, 0},
            {255, 77, 83, 88},
            {255, 26, 30, 34},
            {255, 84, 91, 96},
            {255, 25, 29, 33},
            {255, 12, 14, 16},
            {120, 220, 230, 236},
            {255, 240, 242, 244},
            {255, 48, 53, 57},
            4.0f, 1.02f, 0.44f, 0.382f, 0.17f, 0.315f, 3.0f, true
        },
        {
            L"Brushed Aluminium",
            {80, 0, 0, 0},
            {255, 127, 132, 136},
            {255, 63, 67, 70},
            {255, 221, 224, 226},
            {255, 107, 112, 116},
            {255, 63, 66, 68},
            {145, 255, 255, 255},
            {255, 34, 36, 38},
            {255, 147, 151, 154},
            3.0f, 1.02f, 0.44f, 0.385f, 0.18f, 0.32f, 3.0f, true
        },
        {
            L"Industrial Steel",
            {105, 0, 0, 0},
            {255, 66, 67, 67},
            {255, 18, 19, 20},
            {255, 105, 105, 101},
            {255, 39, 40, 39},
            {255, 9, 9, 9},
            {100, 232, 230, 220},
            {255, 242, 230, 190},
            {255, 55, 55, 52},
            5.0f, 1.03f, 0.45f, 0.38f, 0.16f, 0.325f, 3.5f, true
        },
        {
            L"Minimal Dark",
            {65, 0, 0, 0},
            {255, 38, 40, 43},
            {255, 25, 27, 30},
            {255, 53, 56, 60},
            {255, 31, 33, 36},
            {255, 20, 22, 24},
            {75, 255, 255, 255},
            {255, 214, 220, 226},
            {255, 36, 39, 42},
            2.0f, 1.01f, 0.42f, 0.37f, 0.19f, 0.31f, 2.5f, false
        }
    };
}

bool KnobRenderer::renderVerticalFilmstrip(
    const KnobStyle& style,
    const RenderOptions& options,
    const std::wstring& outputPath) const {

    if (options.cellSize < 16 || options.frameCount < 2) {
        return false;
    }

    // Avoid integer overflow and unreasonable allocations.
    if (options.cellSize > 2048 ||
        options.frameCount > 512 ||
        options.cellSize > (INT_MAX / options.frameCount)) {
        return false;
    }

    GdiPlusSession session;
    if (!session.ok()) {
        return false;
    }

    const int width = options.cellSize;
    const int height = options.cellSize * options.frameCount;

    Bitmap bitmap(width, height, PixelFormat32bppARGB);
    if (bitmap.GetLastStatus() != Ok) {
        return false;
    }

    Graphics g(&bitmap);
    if (g.GetLastStatus() != Ok) {
        return false;
    }

    g.Clear(Gdiplus::Color(0, 0, 0, 0));

    for (int i = 0; i < options.frameCount; ++i) {
        const float t = static_cast<float>(i) /
                        static_cast<float>(options.frameCount - 1);
        const float angle =
            options.startAngleDeg +
            t * (options.endAngleDeg - options.startAngleDeg);

        drawFrame(
            g,
            style,
            0,
            i * options.cellSize,
            options.cellSize,
            angle);
    }

    CLSID pngClsid {};
    if (getEncoderClsid(L"image/png", &pngClsid) < 0) {
        return false;
    }

    return bitmap.Save(outputPath.c_str(), &pngClsid, nullptr) == Ok;
}

} // namespace knob125a
