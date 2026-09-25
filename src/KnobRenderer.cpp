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

PointF polar(float cx, float cy, float radius, float angleDeg) {
    const float rad = (angleDeg - 90.0f) * kPi / 180.0f;
    return PointF(cx + std::cos(rad) * radius, cy + std::sin(rad) * radius);
}

void drawBrushedRing(
    Graphics& g,
    float cx,
    float cy,
    float outerR,
    const Color& light,
    const Color& dark,
    float scale) {

    for (int i = 0; i < 14; ++i) {
        const float inset = (2.0f + static_cast<float>(i) * 0.62f) * scale;
        const float r = outerR - inset;
        if (r <= 1.0f) {
            break;
        }

        const auto& c = (i % 2 == 0) ? light : dark;
        Pen pen(gdipColor(c), std::max(0.55f, 0.55f * scale));
        g.DrawArc(&pen, centeredCircle(cx, cy, r), 202.0f, 126.0f);
        g.DrawArc(&pen, centeredCircle(cx, cy, r), 22.0f, 72.0f);
    }
}

void drawScaleTicks(
    Graphics& g,
    float cx,
    float cy,
    float half,
    const KnobStyle& style,
    float scale) {

    Pen tickPen(gdipColor(style.scaleTick), std::max(1.0f, 1.35f * scale));
    tickPen.SetStartCap(LineCapRound);
    tickPen.SetEndCap(LineCapRound);

    constexpr int tickCount = 13;
    for (int i = 0; i < tickCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(tickCount - 1);
        const float angle = -135.0f + t * 270.0f;
        const bool major = (i == 0 || i == tickCount / 2 || i == tickCount - 1);
        const float rOuter = half * (style.bezelRadius - 0.012f);
        const float rInner = rOuter - half * (major ? 0.060f : 0.038f);
        g.DrawLine(
            &tickPen,
            polar(cx, cy, rInner, angle),
            polar(cx, cy, rOuter, angle));
    }
}

void drawKnurling(
    Graphics& g,
    float cx,
    float cy,
    float half,
    const KnobStyle& style,
    float scale) {

    const float rOuter = half * style.bodyRadius * 1.035f;
    const float rInner = rOuter - std::max(2.0f * scale, half * 0.030f);

    for (int i = 0; i < 48; ++i) {
        const float angle = static_cast<float>(i) * 360.0f / 48.0f;
        const bool lit = (i % 2 == 0);
        Pen pen(
            gdipColor(lit ? style.knurlHighlight : style.knurlShadow),
            std::max(0.8f, 1.0f * scale));
        g.DrawLine(
            &pen,
            polar(cx, cy, rInner, angle),
            polar(cx, cy, rOuter, angle));
    }
}

void drawFrame(
    Graphics& g,
    const KnobStyle& style,
    int xOffset,
    int yOffset,
    int cellSize,
    float angleDeg) {

    const float size = static_cast<float>(cellSize);
    const float scale = size / 128.0f;
    const float cx = static_cast<float>(xOffset) + size * 0.5f;
    const float cy = static_cast<float>(yOffset) + size * 0.5f;
    const float half = size * 0.5f;

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    // Cast shadow.
    {
        const float r = half * style.bezelRadius * style.shadowScale;
        SolidBrush brush(gdipColor(style.shadow));
        g.FillEllipse(&brush, centeredCircle(cx, cy + style.shadowOffsetY * scale, r));
    }

    // Outer bezel.
    {
        const float r = half * style.bezelRadius;
        LinearGradientBrush brush(
            PointF(cx, cy - r),
            PointF(cx, cy + r),
            gdipColor(style.bezelOuter),
            gdipColor(style.bezelInner));
        g.FillEllipse(&brush, centeredCircle(cx, cy, r));

        Pen edgePen(gdipColor(style.edge), std::max(1.0f, scale));
        g.DrawEllipse(&edgePen, centeredCircle(cx, cy, r));

        if (style.drawBrushedBezel) {
            drawBrushedRing(
                g, cx, cy, r,
                {44, 235, 238, 240},
                {36, 0, 0, 0},
                scale);
        }
    }

    if (style.drawScaleTicks) {
        drawScaleTicks(g, cx, cy, half, style, scale);
    }

    // Recess between bezel and knob body.
    {
        const float recessR = half * (style.bodyRadius + 0.050f);
        Pen recessDark(Gdiplus::Color(190, 0, 0, 0), std::max(2.0f, 3.0f * scale));
        g.DrawEllipse(&recessDark, centeredCircle(cx, cy, recessR));

        Pen recessHi(Gdiplus::Color(58, 230, 235, 240), std::max(0.75f, scale));
        g.DrawArc(&recessHi, centeredCircle(cx, cy, recessR - 1.5f * scale), 198.0f, 128.0f);
    }

    // Warm metal accent ring.
    if (style.drawAccentRing) {
        const float r = half * (style.bodyRadius + 0.030f);
        Pen accentDark(Gdiplus::Color(210, 54, 43, 29), std::max(3.0f, 5.8f * scale));
        g.DrawEllipse(&accentDark, centeredCircle(cx, cy, r));

        Pen accent(gdipColor(style.accentRing), std::max(1.0f, 2.4f * scale));
        g.DrawArc(&accent, centeredCircle(cx, cy, r), 205.0f, 118.0f);
        g.DrawArc(&accent, centeredCircle(cx, cy, r), 328.0f, 76.0f);
    }

    // Knob body.
    {
        const float r = half * style.bodyRadius;
        LinearGradientBrush brush(
            PointF(cx - r * 0.4f, cy - r),
            PointF(cx + r * 0.3f, cy + r),
            gdipColor(style.bodyTop),
            gdipColor(style.bodyBottom));
        g.FillEllipse(&brush, centeredCircle(cx, cy, r));

        if (style.drawKnurling) {
            drawKnurling(g, cx, cy, half, style, scale);
        }

        Pen hiPen(gdipColor(style.highlight), std::max(1.0f, size * 0.018f));
        const RectF hiRect = centeredCircle(cx, cy, r * 0.90f);
        g.DrawArc(&hiPen, hiRect, 205.0f, 112.0f);

        // Inner depth rings make the face look machined instead of flat.
        Pen innerDark(Gdiplus::Color(150, 0, 0, 0), std::max(1.0f, 1.4f * scale));
        g.DrawEllipse(&innerDark, centeredCircle(cx, cy, r * 0.86f));

        Pen innerHi(Gdiplus::Color(45, 255, 255, 255), std::max(0.6f, 0.8f * scale));
        g.DrawArc(&innerHi, centeredCircle(cx, cy, r * 0.82f), 205.0f, 110.0f);
    }

    // Indicator.
    {
        const float r1 = half * style.indicatorInnerRadius;
        const float r2 = half * style.indicatorOuterRadius;
        const PointF p1 = polar(cx, cy, r1, angleDeg);
        const PointF p2 = polar(cx, cy, r2, angleDeg);

        // Small dark under-stroke gives the red marker real depth.
        Pen underPen(Gdiplus::Color(190, 0, 0, 0),
                     std::max(2.0f, (style.indicatorWidth + 2.0f) * scale));
        underPen.SetStartCap(LineCapRound);
        underPen.SetEndCap(LineCapRound);
        g.DrawLine(&underPen, p1, p2);

        Pen indicatorPen(
            gdipColor(style.indicator),
            std::max(1.0f, style.indicatorWidth * scale));
        indicatorPen.SetStartCap(LineCapRound);
        indicatorPen.SetEndCap(LineCapRound);
        g.DrawLine(&indicatorPen, p1, p2);

        if (style.drawPointerTip) {
            const PointF tip = polar(cx, cy, r2, angleDeg);
            const float tipR = std::max(2.3f, 3.3f * scale);
            SolidBrush darkTip(Gdiplus::Color(220, 18, 14, 10));
            g.FillEllipse(&darkTip, centeredCircle(tip.X, tip.Y, tipR + 1.1f * scale));
            SolidBrush tipBrush(gdipColor(style.pointerTip));
            g.FillEllipse(&tipBrush, centeredCircle(tip.X, tip.Y, tipR));
        }
    }

    // Center cap goes on top of the pointer root.
    if (style.drawCenterCap) {
        const float capR = half * 0.105f;
        SolidBrush cap(gdipColor(style.centerCap));
        g.FillEllipse(&cap, centeredCircle(cx, cy, capR));

        Pen capHi(Gdiplus::Color(95, 255, 255, 255), std::max(0.7f, 0.8f * scale));
        g.DrawArc(&capHi, centeredCircle(cx, cy, capR * 0.86f), 205.0f, 115.0f);
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

enum class MixEngineKnobSize { Small, Medium, Large };

KnobStyle makeMixEngineAnalog(MixEngineKnobSize variant) {
    const bool small = variant == MixEngineKnobSize::Small;
    const bool large = variant == MixEngineKnobSize::Large;

    KnobStyle s {
        small ? L"MixEngine Analog S" :
        (large ? L"MixEngine Analog L" : L"MixEngine Analog M"),
        {static_cast<std::uint8_t>(large ? 125 : 110), 0, 0, 0},
        {255, 49, 54, 60},
        {255, 13, 16, 20},
        {255, 49, 51, 53},
        {255, 10, 12, 15},
        {255, 4, 5, 7},
        {static_cast<std::uint8_t>(large ? 120 : 100), 226, 232, 238},
        {255, 224, 54, 43},
        {255, 27, 29, 31},
        large ? 5.5f : (small ? 3.0f : 4.5f),
        large ? 1.035f : 1.025f,
        large ? 0.475f : (small ? 0.430f : 0.458f),
        large ? 0.340f : (small ? 0.355f : 0.350f),
        small ? 0.095f : 0.080f,
        large ? 0.300f : (small ? 0.292f : 0.302f),
        large ? 3.8f : (small ? 3.1f : 3.4f),
        true
    };

    s.accentRing = large
        ? Color{255, 166, 134, 82}
        : Color{255, 151, 121, 74};
    s.scaleTick = {255, 224, 214, 194};
    s.knurlHighlight = {82, 182, 188, 194};
    s.knurlShadow = {165, 0, 0, 0};
    s.pointerTip = {255, 243, 224, 183};
    s.drawAccentRing = true;
    // Small utility controls stay cleaner at tiny display sizes.
    s.drawScaleTicks = !small;
    s.drawKnurling = !small;
    s.drawPointerTip = true;
    s.drawBrushedBezel = !small;
    return s;
}

} // namespace

std::vector<KnobStyle> KnobRenderer::builtInStyles() {
    return {
        makeMixEngineAnalog(MixEngineKnobSize::Large),
        makeMixEngineAnalog(MixEngineKnobSize::Medium),
        makeMixEngineAnalog(MixEngineKnobSize::Small),
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
