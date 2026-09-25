#include "HardwareRenderer.h"

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
    const float yShift = pressed ? half * 0.035f : 0.0f;
    const float outer = half * 0.43f;
    const float face = half * (pressed ? 0.32f : 0.34f);

    SolidBrush shadow(gc(s.shadow));
    g.FillEllipse(&shadow,
                  RectF(cx - outer, cy - outer + half * 0.08f,
                        outer * 2.0f, outer * 2.0f));

    LinearGradientBrush bezel(
        PointF(cx, cy - outer), PointF(cx, cy + outer),
        gc(s.metalTop), gc(s.metalBottom));
    g.FillEllipse(&bezel, RectF(cx - outer, cy - outer, outer * 2.0f, outer * 2.0f));

    LinearGradientBrush faceBrush(
        PointF(cx, cy - face + yShift),
        PointF(cx, cy + face + yShift),
        hover ? Gdiplus::Color(255, 74, 77, 80) : gc(s.faceTop),
        pressed ? Gdiplus::Color(255, 9, 10, 12) : gc(s.faceBottom));
    g.FillEllipse(&faceBrush,
                  RectF(cx - face, cy - face + yShift, face * 2.0f, face * 2.0f));

    Pen rim(Gdiplus::Color(190, 0, 0, 0), std::max(1.0f, half * 0.032f));
    g.DrawEllipse(&rim, RectF(cx - face, cy - face + yShift, face * 2.0f, face * 2.0f));

    if (!pressed) {
        Pen hi(gc(s.highlight), std::max(0.8f, half * 0.025f));
        g.DrawArc(&hi, RectF(cx - face * 0.86f, cy - face * 0.86f + yShift,
                            face * 1.72f, face * 1.72f), 205.0f, 105.0f);
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
    const RectF base(cx - baseW, cy - baseH, baseW * 2.0f, baseH * 2.0f);

    LinearGradientBrush bezel(
        PointF(cx, cy - baseH), PointF(cx, cy + baseH),
        gc(s.metalTop), gc(s.metalBottom));
    g.FillRectangle(&bezel, base);

    Pen edge(Gdiplus::Color(210, 2, 3, 4), std::max(1.0f, half * 0.028f));
    g.DrawRectangle(&edge, base);

    if (rocker) {
        const float inset = half * 0.09f;
        const RectF face(base.X + inset, base.Y + inset,
                         base.Width - inset * 2.0f, base.Height - inset * 2.0f);
        LinearGradientBrush fb(
            PointF(cx, face.Y), PointF(cx, face.GetBottom()),
            on ? gc(s.faceBottom) : gc(s.faceTop),
            on ? gc(s.faceTop) : gc(s.faceBottom));
        g.FillRectangle(&fb, face);

        Pen center(Gdiplus::Color(110, 0, 0, 0), std::max(1.0f, half * 0.02f));
        g.DrawLine(&center, PointF(face.X, cy), PointF(face.GetRight(), cy));

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

    const float dx = on ? half * 0.14f : -half * 0.14f;
    const float dy = on ? -half * 0.24f : half * 0.24f;
    Pen stem(Gdiplus::Color(255, 174, 177, 180), std::max(2.0f, half * 0.10f));
    stem.SetStartCap(LineCapRound);
    stem.SetEndCap(LineCapRound);
    g.DrawLine(&stem, PointF(cx, cy), PointF(cx + dx, cy + dy));

    SolidBrush tip(Gdiplus::Color(255, 56, 58, 60));
    const float tr = half * 0.13f;
    g.FillEllipse(&tip, RectF(cx + dx - tr, cy + dy - tr, tr * 2.0f, tr * 2.0f));
}

void drawState(
    Graphics& g,
    const HardwareAssetStyle& s,
    int yOffset,
    int cellSize,
    int state) {

    setup(g);
    const float size = static_cast<float>(cellSize);
    const float half = size * 0.5f;
    const float cx = half;
    const float cy = static_cast<float>(yOffset) + half;

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
    ledRed.kind = HardwareAssetKind::Led;
    ledRed.preferredCellSize = 32;
    ledRed.stateCount = 2;

    HardwareAssetStyle ledAmber = ledRed;
    ledAmber.name = L"MixEngine LED Amber";
    ledAmber.accentOff = {255, 62, 44, 16};
    ledAmber.accentOn = {255, 242, 181, 63};

    HardwareAssetStyle button;
    button.name = L"MixEngine Push Button";
    button.kind = HardwareAssetKind::PushButton;
    button.preferredCellSize = 64;
    button.stateCount = 3;

    HardwareAssetStyle toggle;
    toggle.name = L"MixEngine Toggle";
    toggle.kind = HardwareAssetKind::ToggleSwitch;
    toggle.preferredCellSize = 64;
    toggle.stateCount = 2;

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
