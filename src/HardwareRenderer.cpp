#include "HardwareRenderer.h"
#include "AssetGeometry.h"

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

RectF square(float cx, float cy, float half) {
    return RectF(cx - half, cy - half, half * 2.0f, half * 2.0f);
}

void buildRoundedRect(
    GraphicsPath& path,
    float x,
    float y,
    float w,
    float h,
    float radius) {

    path.Reset();
    const float d = radius * 2.0f;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void setup(Graphics& g) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
}

void drawLed(
    Graphics& g,
    const HardwareAssetStyle& s,
    float cx,
    float cy,
    float half,
    bool on) {

    const float ringR = half * 0.44f;
    const float lensR = half * 0.29f;

    SolidBrush shadow(gc(s.shadow));
    g.FillEllipse(&shadow, RectF(cx - ringR, cy - ringR + half * 0.06f,
                                ringR * 2.0f, ringR * 2.0f));

    LinearGradientBrush ring(
        PointF(cx, cy - ringR), PointF(cx, cy + ringR),
        gc(s.metalTop), gc(s.metalBottom));
    g.FillEllipse(&ring, RectF(cx - ringR, cy - ringR, ringR * 2.0f, ringR * 2.0f));

    Pen edge(Gdiplus::Color(210, 4, 5, 6), std::max(1.0f, half * 0.035f));
    g.DrawEllipse(&edge, RectF(cx - ringR, cy - ringR, ringR * 2.0f, ringR * 2.0f));

    const Color lens = on ? s.accentOn : s.accentOff;
    LinearGradientBrush lensBrush(
        PointF(cx - lensR * 0.35f, cy - lensR),
        PointF(cx + lensR * 0.25f, cy + lensR),
        on ? Gdiplus::Color(255, std::min(255, lens.r + 18),
                            std::min(255, lens.g + 18),
                            std::min(255, lens.b + 18)) : gc(lens),
        gc(lens));
    g.FillEllipse(&lensBrush,
                  RectF(cx - lensR, cy - lensR, lensR * 2.0f, lensR * 2.0f));

    if (on) {
        SolidBrush glow(Gdiplus::Color(40, s.accentOn.r, s.accentOn.g, s.accentOn.b));
        const float glowR = half * 0.39f;
        g.FillEllipse(&glow, RectF(cx - glowR, cy - glowR, glowR * 2.0f, glowR * 2.0f));
    }

    SolidBrush hi(gc(s.highlight));
    const float hr = lensR * 0.29f;
    g.FillEllipse(&hi, RectF(cx - lensR * 0.52f, cy - lensR * 0.58f,
                            hr, hr * 0.65f));
}

void drawPushButton(
    Graphics& g,
    const HardwareAssetStyle& s,
    float cx,
    float cy,
    float half,
    int state) {

    const bool pressed = state >= 2;
    const bool hover = state == 1;

    const float w = half * 1.48f;
    const float h = half * 0.78f;
    const float x = cx - w * 0.5f;
    const float y = cy - h * 0.5f + (pressed ? half * 0.035f : 0.0f);
    const float r = std::max(2.0f, half * 0.10f);

    GraphicsPath shadowPath;
    buildRoundedRect(
        shadowPath,
        x,
        y + half * 0.07f,
        w,
        h,
        r);

    SolidBrush shadow(gc(s.shadow));
    g.FillPath(&shadow, &shadowPath);

    GraphicsPath bezelPath;
    buildRoundedRect(bezelPath, x, y, w, h, r);
    LinearGradientBrush bezel(
        PointF(cx, y),
        PointF(cx, y + h),
        gc(s.metalTop),
        gc(s.metalBottom));
    g.FillPath(&bezel, &bezelPath);

    const float inset = half * 0.075f;
    GraphicsPath facePath;
    buildRoundedRect(
        facePath,
        x + inset,
        y + inset,
        w - inset * 2.0f,
        h - inset * 2.0f,
        std::max(1.5f, r - inset * 0.4f));

    LinearGradientBrush faceBrush(
        PointF(cx, y + inset),
        PointF(cx, y + h - inset),
        hover ? Gdiplus::Color(255, 64, 72, 79) : gc(s.faceTop),
        pressed ? Gdiplus::Color(255, 10, 12, 15) : gc(s.faceBottom));
    g.FillPath(&faceBrush, &facePath);

    Pen edge(
        Gdiplus::Color(220, 3, 5, 7),
        std::max(0.8f, half * 0.025f));
    g.DrawPath(&edge, &bezelPath);

    if (!pressed) {
        Pen hi(
            hover ? Gdiplus::Color(120, 220, 230, 238)
                  : Gdiplus::Color(64, 220, 230, 238),
            std::max(0.7f, half * 0.018f));
        g.DrawLine(
            &hi,
            PointF(x + r, y + inset),
            PointF(x + w - r, y + inset));
    }
}

void drawToggle(
    Graphics& g,
    const HardwareAssetStyle& s,
    float cx,
    float cy,
    float half,
    bool on,
    bool rocker) {

    const float baseW = half * (rocker ? 0.72f : 0.58f);
    const float baseH = half * (rocker ? 0.88f : 0.62f);
    const float baseX = snapHardEdge(cx - baseW, s.geometry);
    const float baseY = snapHardEdge(cy - baseH, s.geometry);
    const float baseR = snapHardEdge(cx + baseW, s.geometry);
    const float baseB = snapHardEdge(cy + baseH, s.geometry);
    const RectF base(baseX, baseY, baseR - baseX, baseB - baseY);

    if (!rocker) {
        const float bezelR = half * 0.40f;
        SolidBrush shadow(gc(s.shadow));
        g.FillEllipse(
            &shadow,
            RectF(cx - bezelR, cy - bezelR + half * 0.055f,
                  bezelR * 2.0f, bezelR * 2.0f));

        LinearGradientBrush roundBezel(
            PointF(cx, cy - bezelR), PointF(cx, cy + bezelR),
            gc(s.metalTop), gc(s.metalBottom));
        g.FillEllipse(
            &roundBezel,
            RectF(cx - bezelR, cy - bezelR, bezelR * 2.0f, bezelR * 2.0f));

        Pen roundEdge(
            Gdiplus::Color(220, 2, 3, 4),
            std::max(1.0f, half * 0.028f));
        g.DrawEllipse(
            &roundEdge,
            RectF(cx - bezelR, cy - bezelR, bezelR * 2.0f, bezelR * 2.0f));
    } else {
        LinearGradientBrush bezel(
            PointF(cx, cy - baseH), PointF(cx, cy + baseH),
            gc(s.metalTop), gc(s.metalBottom));
        g.FillRectangle(&bezel, base);

        Pen edge(
            Gdiplus::Color(210, 2, 3, 4),
            std::max(1.0f, half * 0.028f));
        g.DrawRectangle(&edge, base);
    }

    if (rocker) {
        const float inset = half * 0.09f;
        const RectF face(base.X + inset, base.Y + inset,
                         base.Width - inset * 2.0f, base.Height - inset * 2.0f);
        LinearGradientBrush fb(
            PointF(cx, face.Y), PointF(cx, face.GetBottom()),
            on ? gc(s.faceBottom) : gc(s.faceTop),
            on ? gc(s.faceTop) : gc(s.faceBottom));
        g.FillRectangle(&fb, face);

        Pen center(Gdiplus::Color(135, 0, 0, 0), std::max(1.0f, half * 0.022f));
        g.DrawLine(&center, PointF(face.X, cy), PointF(face.GetRight(), cy));

        Pen bevel(
            Gdiplus::Color(on ? 40 : 95, 255, 255, 255),
            std::max(0.8f, half * 0.020f));
        g.DrawLine(
            &bevel,
            PointF(face.X + inset, on ? face.GetBottom() - inset : face.Y + inset),
            PointF(face.GetRight() - inset, on ? face.GetBottom() - inset : face.Y + inset));

        const float markR = half * 0.055f;
        SolidBrush lamp(gc(on ? s.accentOn : s.accentOff));
        g.FillEllipse(&lamp, RectF(cx - markR, face.Y + inset,
                                  markR * 2.0f, markR * 2.0f));
        return;
    }

    const float pivotR = half * 0.18f;
    SolidBrush pivot(Gdiplus::Color(255, 35, 38, 41));
    g.FillEllipse(&pivot,
                  RectF(cx - pivotR, cy - pivotR, pivotR * 2.0f, pivotR * 2.0f));

    const float dx = on ? half * 0.15f : -half * 0.15f;
    const float dy = on ? -half * 0.27f : half * 0.27f;
    Pen stem(Gdiplus::Color(255, 194, 198, 201), std::max(2.0f, half * 0.085f));
    stem.SetStartCap(LineCapRound);
    stem.SetEndCap(LineCapRound);
    g.DrawLine(&stem, PointF(cx, cy), PointF(cx + dx, cy + dy));

    SolidBrush tip(on ? Gdiplus::Color(255, 68, 70, 72)
                      : Gdiplus::Color(255, 48, 50, 52));
    const float tr = half * 0.12f;
    g.FillEllipse(&tip, RectF(cx + dx - tr, cy + dy - tr, tr * 2.0f, tr * 2.0f));
}

void drawState(
    Graphics& g,
    const HardwareAssetStyle& s,
    int yOffset,
    int cellSize,
    int state) {

    setup(g);
    const auto metrics = makeAssetMetrics(
        cellSize, 0, yOffset, s.geometry);
    const float half = metrics.safeHalf;
    const float cx = metrics.cx;
    const float cy = metrics.cy;

    switch (s.kind) {
        case HardwareAssetKind::Led:
            drawLed(g, s, cx, cy, half, state != 0);
            break;
        case HardwareAssetKind::PushButton:
            drawPushButton(g, s, cx, cy, half, state);
            break;
        case HardwareAssetKind::ToggleSwitch:
            drawToggle(g, s, cx, cy, half, state != 0, false);
            break;
        case HardwareAssetKind::RockerSwitch:
            drawToggle(g, s, cx, cy, half, state != 0, true);
            break;
    }
}

} // namespace

std::vector<HardwareAssetStyle> HardwareRenderer::builtInStyles() {
    HardwareAssetStyle ledRed;
    ledRed.name = L"MixEngine LED Red";
    ledRed.geometry.safeAreaRatio = 0.84f;
    ledRed.kind = HardwareAssetKind::Led;
    ledRed.preferredCellSize = 32;
    ledRed.stateCount = 2;

    HardwareAssetStyle ledAmber = ledRed;
    ledAmber.name = L"MixEngine LED Amber";
    ledAmber.accentOff = {255, 62, 44, 16};
    ledAmber.accentOn = {255, 242, 181, 63};

    HardwareAssetStyle button;
    button.name = L"MixEngine Push Button";
    button.geometry.safeAreaRatio = 0.88f;
    button.kind = HardwareAssetKind::PushButton;
    button.preferredCellSize = 64;
    button.stateCount = 3;
    button.metalTop = {255, 68, 78, 88};
    button.metalBottom = {255, 19, 24, 29};
    button.faceTop = {255, 41, 49, 57};
    button.faceBottom = {255, 13, 17, 21};
    button.highlight = {82, 220, 230, 238};

    HardwareAssetStyle toggle;
    toggle.name = L"MixEngine Toggle";
    toggle.geometry.safeAreaRatio = 0.86f;
    toggle.kind = HardwareAssetKind::ToggleSwitch;
    toggle.preferredCellSize = 64;
    toggle.stateCount = 2;
    toggle.metalTop = {255, 68, 78, 88};
    toggle.metalBottom = {255, 19, 24, 29};
    toggle.faceTop = {255, 41, 49, 57};
    toggle.faceBottom = {255, 13, 17, 21};
    toggle.highlight = {82, 220, 230, 238};

    HardwareAssetStyle rocker = toggle;
    rocker.name = L"MixEngine Rocker";
    rocker.kind = HardwareAssetKind::RockerSwitch;
    rocker.stateCount = 2;

    return {ledRed, ledAmber, button, toggle, rocker};
}

bool HardwareRenderer::renderVerticalFilmstrip(
    const HardwareAssetStyle& style,
    int cellSize,
    const std::wstring& outputPath) const {

    if (cellSize < 16 || cellSize > 2048 ||
        style.stateCount < 2 || style.stateCount > 64 ||
        cellSize > (INT_MAX / style.stateCount)) {
        return false;
    }

    GdiPlusSession session;
    if (!session.ok()) {
        return false;
    }

    Bitmap bitmap(cellSize, cellSize * style.stateCount, PixelFormat32bppARGB);
    if (bitmap.GetLastStatus() != Ok) {
        return false;
    }

    Graphics g(&bitmap);
    if (g.GetLastStatus() != Ok) {
        return false;
    }

    g.Clear(Gdiplus::Color(0, 0, 0, 0));
    for (int state = 0; state < style.stateCount; ++state) {
        drawState(g, style, state * cellSize, cellSize, state);
    }

    CLSID pngClsid;
    if (getEncoderClsid(L"image/png", &pngClsid) < 0) {
        return false;
    }

    return bitmap.Save(outputPath.c_str(), &pngClsid, nullptr) == Ok;
}

} // namespace knob125a
