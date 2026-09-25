#include "MeterRenderer.h"

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

Gdiplus::Color gc(const knob125a::Color& c) {
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

PointF polar(float cx, float cy, float radius, float angleDeg) {
    const float rad = (angleDeg - 90.0f) * kPi / 180.0f;
    return PointF(
        cx + std::cos(rad) * radius,
        cy + std::sin(rad) * radius);
}

void drawMeterFrame(
    Graphics& g,
    const MeterStyle& s,
    int yOffset,
    int cellWidth,
    int cellHeight,
    float value) {

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    const float w = static_cast<float>(cellWidth);
    const float h = static_cast<float>(cellHeight);
    const float scale = std::min(
        w / static_cast<float>(s.preferredWidth),
        h / static_cast<float>(s.preferredHeight));

    const float safe = std::clamp(
        s.geometry.safeAreaRatio, 0.60f, 0.98f);
    const float marginX = w * (1.0f - safe) * 0.5f;
    const float marginY = h * (1.0f - safe) * 0.5f;

    const float x = marginX;
    const float y = static_cast<float>(yOffset) + marginY;
    const float fw = w - marginX * 2.0f;
    const float fh = h - marginY * 2.0f;

    SolidBrush shadow(gc(s.shadow));
    g.FillRectangle(
        &shadow,
        RectF(x + 1.5f * scale, y + 2.0f * scale, fw, fh));

    LinearGradientBrush frame(
        PointF(x, y), PointF(x, y + fh),
        gc(s.frameTop), gc(s.frameBottom));
    g.FillRectangle(&frame, RectF(x, y, fw, fh));

    Pen edge(Gdiplus::Color(220, 4, 5, 6),
             std::max(1.0f, 1.1f * scale));
    g.DrawRectangle(&edge, RectF(x, y, fw, fh));

    const float inset = std::max(4.0f * scale, w * 0.034f);
    const RectF faceRect(
        x + inset,
        y + inset,
        fw - inset * 2.0f,
        fh - inset * 2.0f);

    LinearGradientBrush face(
        PointF(faceRect.X, faceRect.Y),
        PointF(faceRect.X, faceRect.GetBottom()),
        gc(s.faceTop), gc(s.faceBottom));
    g.FillRectangle(&face, faceRect);

    Pen faceEdge(
        Gdiplus::Color(170, 45, 38, 29),
        std::max(0.8f, 0.8f * scale));
    g.DrawRectangle(&faceEdge, faceRect);

    const float cx = faceRect.X + faceRect.Width * 0.5f;
    const float pivotY = faceRect.Y + faceRect.Height * 0.91f;
    const float radius = std::min(
        faceRect.Width * 0.42f,
        faceRect.Height * 0.83f);

    Pen scalePen(gc(s.scale), std::max(0.8f, 0.9f * scale));
    Pen redPen(gc(s.redZone), std::max(1.0f, 1.2f * scale));

    constexpr int tickCount = 13;
    for (int i = 0; i < tickCount; ++i) {
        const float t =
            static_cast<float>(i) /
            static_cast<float>(tickCount - 1);
        const float angle =
            s.startAngleDeg +
            t * (s.endAngleDeg - s.startAngleDeg);

        const bool major = (i % 3) == 0;
        const float outerR = radius;
        const float innerR = radius -
            (major ? 9.0f : 6.0f) * scale;

        Pen& pen = i >= tickCount - 3 ? redPen : scalePen;
        g.DrawLine(
            &pen,
            polar(cx, pivotY, innerR, angle),
            polar(cx, pivotY, outerR, angle));
    }

    // Fine inner arc suggests a calibrated analog meter face.
    Pen arcPen(
        Gdiplus::Color(120, s.scale.r, s.scale.g, s.scale.b),
        std::max(0.6f, 0.65f * scale));
    const float arcR = radius - 13.0f * scale;
    const RectF arcRect(
        cx - arcR,
        pivotY - arcR,
        arcR * 2.0f,
        arcR * 2.0f);
    g.DrawArc(
        &arcPen,
        arcRect,
        218.0f,
        104.0f);

    const float needleAngle =
        s.startAngleDeg +
        std::clamp(value, 0.0f, 1.0f) *
        (s.endAngleDeg - s.startAngleDeg);

    Pen needleShadow(
        Gdiplus::Color(95, 0, 0, 0),
        std::max(1.5f, 2.0f * scale));
    g.DrawLine(
        &needleShadow,
        PointF(cx + 1.0f * scale, pivotY + 1.0f * scale),
        polar(cx + 1.0f * scale, pivotY + 1.0f * scale,
              radius * 0.83f, needleAngle));

    Pen needle(
        gc(s.needle),
        std::max(0.9f, 1.1f * scale));
    g.DrawLine(
        &needle,
        PointF(cx, pivotY),
        polar(cx, pivotY, radius * 0.83f, needleAngle));

    SolidBrush hub(Gdiplus::Color(255, 34, 31, 27));
    const float hubR = std::max(3.0f, 4.1f * scale);
    g.FillEllipse(
        &hub,
        RectF(cx - hubR, pivotY - hubR,
              hubR * 2.0f, hubR * 2.0f));

    // Subtle glass reflection.
    LinearGradientBrush glass(
        PointF(faceRect.X, faceRect.Y),
        PointF(faceRect.X, faceRect.Y + faceRect.Height * 0.45f),
        gc(s.glass),
        Gdiplus::Color(0, 255, 255, 255));
    g.FillRectangle(
        &glass,
        RectF(
            faceRect.X + 1.0f * scale,
            faceRect.Y + 1.0f * scale,
            faceRect.Width - 2.0f * scale,
            faceRect.Height * 0.34f));
}

} // namespace

std::vector<MeterStyle> MeterRenderer::builtInStyles() {
    MeterStyle vu;
    vu.name = L"MixEngine VU Meter";
    vu.preferredWidth = 160;
    vu.preferredHeight = 92;
    vu.frameCount = 128;
    vu.geometry.safeAreaRatio = 0.92f;
    vu.frameTop = {255, 55, 65, 74};
    vu.frameBottom = {255, 13, 18, 23};
    vu.faceTop = {255, 225, 215, 188};
    vu.faceBottom = {255, 188, 174, 143};
    vu.scale = {255, 48, 45, 40};
    vu.redZone = {255, 162, 49, 42};
    vu.needle = {255, 48, 42, 36};
    vu.glass = {28, 255, 255, 255};
    vu.shadow = {95, 0, 0, 0};
    return {vu};
}

bool MeterRenderer::renderVerticalFilmstrip(
    const MeterStyle& style,
    int cellWidth,
    int cellHeight,
    int frameCount,
    const std::wstring& outputPath) const {

    if (cellWidth < 32 || cellHeight < 24 ||
        cellWidth > 2048 || cellHeight > 2048 ||
        frameCount < 2 || frameCount > 512 ||
        cellHeight > (INT_MAX / frameCount) ||
        cellHeight * frameCount > 65535) {
        return false;
    }

    GdiPlusSession session;
    if (!session.ok()) {
        return false;
    }

    Bitmap bitmap(
        cellWidth,
        cellHeight * frameCount,
        PixelFormat32bppARGB);
    if (bitmap.GetLastStatus() != Ok) {
        return false;
    }

    Graphics g(&bitmap);
    if (g.GetLastStatus() != Ok) {
        return false;
    }

    g.Clear(Gdiplus::Color(0, 0, 0, 0));

    for (int i = 0; i < frameCount; ++i) {
        const float value =
            static_cast<float>(i) /
            static_cast<float>(frameCount - 1);
        drawMeterFrame(
            g, style, i * cellHeight,
            cellWidth, cellHeight, value);
    }

    CLSID pngClsid;
    if (getEncoderClsid(L"image/png", &pngClsid) < 0) {
        return false;
    }

    return bitmap.Save(outputPath.c_str(), &pngClsid, nullptr) == Ok;
}

} // namespace knob125a
