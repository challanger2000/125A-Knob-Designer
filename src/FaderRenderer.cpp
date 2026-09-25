#include "FaderRenderer.h"

#include <windows.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace knob125a {
namespace {

using namespace Gdiplus;

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

GraphicsPath roundedRectPath(float x, float y, float w, float h, float radius) {
    GraphicsPath path;
    const float d = radius * 2.0f;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
    return path;
}

void drawFaderFrame(
    Graphics& g,
    const FaderStyle& s,
    int yOffset,
    int cellWidth,
    int cellHeight,
    float value) {

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    const float w = static_cast<float>(cellWidth);
    const float h = static_cast<float>(cellHeight);
    const float sx = w / static_cast<float>(s.preferredWidth);
    const float sy = h / static_cast<float>(s.preferredHeight);
    const float scale = std::min(sx, sy);

    const float cx = w * 0.5f + s.geometry.opticalOffsetX * w * 0.5f;
    const float safe = std::clamp(s.geometry.safeAreaRatio, 0.55f, 0.98f);
    const float top = static_cast<float>(yOffset) + h * (1.0f - safe) * 0.5f;
    const float bottom = static_cast<float>(yOffset) + h -
                         h * (1.0f - safe) * 0.5f;

    const float capW = w * 0.66f;
    const float capH = std::max(12.0f * scale, h * 0.105f);
    const float travelTop = top + capH * 0.70f;
    const float travelBottom = bottom - capH * 0.70f;
    const float capY = travelBottom -
        std::clamp(value, 0.0f, 1.0f) * (travelBottom - travelTop);

    Pen tickPen(gc(s.tick), std::max(0.75f, 0.85f * scale));
    for (int i = 0; i < 11; ++i) {
        const float t = static_cast<float>(i) / 10.0f;
        const float y = travelBottom - t * (travelBottom - travelTop);
        const bool major = (i == 0 || i == 5 || i == 10);
        const float len = w * (major ? 0.16f : 0.10f);
        g.DrawLine(&tickPen, PointF(cx - w * 0.35f, y),
                   PointF(cx - w * 0.35f + len, y));
        g.DrawLine(&tickPen, PointF(cx + w * 0.35f - len, y),
                   PointF(cx + w * 0.35f, y));
    }

    const float recessW = std::max(7.0f * scale, w * 0.13f);
    SolidBrush recess(Gdiplus::Color(205, 0, 0, 0));
    g.FillRectangle(
        &recess,
        RectF(cx - recessW * 0.5f,
              travelTop,
              recessW,
              travelBottom - travelTop));

    const float railW = std::max(2.0f * scale, w * 0.035f);
    LinearGradientBrush rail(
        PointF(cx - railW, travelTop),
        PointF(cx + railW, travelTop),
        gc(s.railOuter),
        gc(s.railInner));
    g.FillRectangle(
        &rail,
        RectF(cx - railW * 0.5f,
              travelTop,
              railW,
              travelBottom - travelTop));

    const float x = cx - capW * 0.5f;
    const float y = capY - capH * 0.5f;
    const float radius = std::max(2.0f, 3.0f * scale);

    SolidBrush shadow(gc(s.shadow));
    auto shadowPath = roundedRectPath(
        x, y + 2.0f * scale, capW, capH, radius);
    g.FillPath(&shadow, &shadowPath);

    auto capPath = roundedRectPath(x, y, capW, capH, radius);
    LinearGradientBrush cap(
        PointF(cx, y),
        PointF(cx, y + capH),
        gc(s.capTop),
        gc(s.capBottom));
    g.FillPath(&cap, &capPath);

    Pen edge(gc(s.capEdge), std::max(0.8f, 1.0f * scale));
    g.DrawPath(&edge, &capPath);

    Pen marker(gc(s.indicator), std::max(1.0f, 1.6f * scale));
    g.DrawLine(&marker,
               PointF(cx - capW * 0.27f, capY),
               PointF(cx + capW * 0.27f, capY));

    Pen hi(Gdiplus::Color(82, 255, 255, 255), std::max(0.7f, 0.8f * scale));
    g.DrawLine(&hi,
               PointF(x + radius, y + capH * 0.22f),
               PointF(x + capW - radius, y + capH * 0.22f));
}

} // namespace

std::vector<FaderStyle> FaderRenderer::builtInStyles() {
    FaderStyle s;
    s.name = L"MixEngine Fader";
    s.preferredWidth = 56;
    s.preferredHeight = 160;
    s.frameCount = 128;
    s.geometry.safeAreaRatio = 0.92f;
    return {s};
}

bool FaderRenderer::renderVerticalFilmstrip(
    const FaderStyle& style,
    int cellWidth,
    int cellHeight,
    int frameCount,
    const std::wstring& outputPath) const {

    if (cellWidth < 16 || cellHeight < 32 ||
        cellWidth > 2048 || cellHeight > 4096 ||
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
        drawFaderFrame(
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
