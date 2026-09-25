#include "KnobRenderer.h"
#include "HardwareRenderer.h"
#include "FaderRenderer.h"
#include "MeterRenderer.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

namespace fs = std::filesystem;
using namespace knob125a;

namespace {

constexpr wchar_t kWindowClass[] = L"125A_GUI_Asset_Designer_Window";

enum ControlId {
    IdCategory = 1001,
    IdPreset = 1002,
    IdScale = 1003,
    IdFrame = 1004,
    IdRender = 1005,
    IdExport = 1006,

    IdSize = 1101,
    IdFrames = 1102,
    IdStartAngle = 1103,
    IdEndAngle = 1104,
    IdTicks = 1105,

    IdAccentToggle = 1110,
    IdTicksToggle = 1111,
    IdKnurlToggle = 1112,
    IdPointerTipToggle = 1113,
    IdBrushedToggle = 1114,

    IdBodyColor = 1120,
    IdBezelColor = 1121,
    IdAccentColor = 1122,
    IdPointerColor = 1123,
    IdResetKnob = 1130
};

struct AppState {
    HWND window {};
    HWND category {};
    HWND preset {};
    HWND scale {};
    HWND frame {};
    HWND status {};

    HWND knobPanel {};
    HWND knobSize {};
    HWND knobFrames {};
    HWND knobStartAngle {};
    HWND knobEndAngle {};
    HWND knobTicks {};
    HWND knobAccentToggle {};
    HWND knobTicksToggle {};
    HWND knobKnurlToggle {};
    HWND knobPointerTipToggle {};
    HWND knobBrushedToggle {};
    HWND knobBodyColor {};
    HWND knobBezelColor {};
    HWND knobAccentColor {};
    HWND knobPointerColor {};
    HWND knobReset {};

    std::unique_ptr<Gdiplus::Image> previewImage;
    fs::path previewPath;

    int frameCount {1};
    int cellWidth {128};
    int cellHeight {128};

    std::vector<KnobStyle> knobStyles;
    std::vector<HardwareAssetStyle> hardwareStyles;
    std::vector<FaderStyle> faderStyles;
    std::vector<MeterStyle> meterStyles;

    KnobStyle editedKnob {};
    RenderOptions editedKnobOptions {};
    bool editedKnobValid {false};
};

AppState gApp;

std::wstring safeFilename(std::wstring name) {
    for (auto& ch : name) {
        if (ch == L' ' || ch == L'/' || ch == L'\\') {
            ch = L'_';
        }
    }
    return name;
}

float selectedScale() {
    const int index = static_cast<int>(
        SendMessageW(gApp.scale, CB_GETCURSEL, 0, 0));
    switch (index) {
        case 1: return 1.5f;
        case 2: return 2.0f;
        case 3: return 3.0f;
        default: return 1.0f;
    }
}

int selectedCategory() {
    const int index = static_cast<int>(
        SendMessageW(gApp.category, CB_GETCURSEL, 0, 0));
    return std::max(0, index);
}

int selectedPreset() {
    const int index = static_cast<int>(
        SendMessageW(gApp.preset, CB_GETCURSEL, 0, 0));
    return std::max(0, index);
}

void setStatus(const std::wstring& text) {
    SetWindowTextW(gApp.status, text.c_str());
}

int readInt(HWND edit, int fallback, int minValue, int maxValue) {
    wchar_t buffer[64] {};
    GetWindowTextW(edit, buffer, 64);
    wchar_t* end = nullptr;
    const long value = wcstol(buffer, &end, 10);
    if (end == buffer) {
        return fallback;
    }
    return std::clamp(static_cast<int>(value), minValue, maxValue);
}

float readFloat(HWND edit, float fallback, float minValue, float maxValue) {
    wchar_t buffer[64] {};
    GetWindowTextW(edit, buffer, 64);
    wchar_t* end = nullptr;
    const double value = wcstod(buffer, &end);
    if (end == buffer) {
        return fallback;
    }
    return std::clamp(static_cast<float>(value), minValue, maxValue);
}

void setEditInt(HWND edit, int value) {
    SetWindowTextW(edit, std::to_wstring(value).c_str());
}

void setEditFloat(HWND edit, float value) {
    wchar_t buffer[64] {};
    swprintf_s(buffer, L"%.1f", static_cast<double>(value));
    SetWindowTextW(edit, buffer);
}

COLORREF toColorRef(const Color& c) {
    return RGB(c.r, c.g, c.b);
}

Color fromColorRef(COLORREF value, std::uint8_t alpha = 255) {
    return {
        alpha,
        GetRValue(value),
        GetGValue(value),
        GetBValue(value)
    };
}

Color darker(Color c, float factor) {
    c.r = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(std::lround(c.r * factor)), 0, 255));
    c.g = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(std::lround(c.g * factor)), 0, 255));
    c.b = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(std::lround(c.b * factor)), 0, 255));
    return c;
}

bool chooseColor(HWND owner, Color& target) {
    static COLORREF custom[16] {};
    CHOOSECOLORW cc {};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = owner;
    cc.rgbResult = toColorRef(target);
    cc.lpCustColors = custom;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (!ChooseColorW(&cc)) {
        return false;
    }

    target = fromColorRef(cc.rgbResult, target.a);
    return true;
}

void applyKnobEditor() {
    if (!gApp.editedKnobValid) {
        return;
    }

    gApp.editedKnob.preferredCellSize =
        readInt(
            gApp.knobSize,
            gApp.editedKnob.preferredCellSize,
            24,
            512);

    gApp.editedKnobOptions.frameCount =
        readInt(
            gApp.knobFrames,
            gApp.editedKnobOptions.frameCount,
            2,
            256);

    gApp.editedKnobOptions.startAngleDeg =
        readFloat(
            gApp.knobStartAngle,
            gApp.editedKnobOptions.startAngleDeg,
            -360.0f,
            360.0f);

    gApp.editedKnobOptions.endAngleDeg =
        readFloat(
            gApp.knobEndAngle,
            gApp.editedKnobOptions.endAngleDeg,
            -360.0f,
            360.0f);

    gApp.editedKnob.tickCount =
        readInt(
            gApp.knobTicks,
            gApp.editedKnob.tickCount,
            3,
            41);

    gApp.editedKnob.drawAccentRing =
        SendMessageW(gApp.knobAccentToggle, BM_GETCHECK, 0, 0) == BST_CHECKED;
    gApp.editedKnob.drawScaleTicks =
        SendMessageW(gApp.knobTicksToggle, BM_GETCHECK, 0, 0) == BST_CHECKED;
    gApp.editedKnob.drawKnurling =
        SendMessageW(gApp.knobKnurlToggle, BM_GETCHECK, 0, 0) == BST_CHECKED;
    gApp.editedKnob.drawPointerTip =
        SendMessageW(gApp.knobPointerTipToggle, BM_GETCHECK, 0, 0) == BST_CHECKED;
    gApp.editedKnob.drawBrushedBezel =
        SendMessageW(gApp.knobBrushedToggle, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void syncKnobEditorFromPreset() {
    const int preset = selectedPreset();
    if (preset < 0 || preset >= static_cast<int>(gApp.knobStyles.size())) {
        gApp.editedKnobValid = false;
        return;
    }

    gApp.editedKnob = gApp.knobStyles[preset];
    gApp.editedKnob.name = gApp.knobStyles[preset].name + L" Custom";

    gApp.editedKnobOptions = {};
    gApp.editedKnobOptions.cellSize = gApp.editedKnob.preferredCellSize;
    gApp.editedKnobOptions.frameCount = 128;
    gApp.editedKnobOptions.startAngleDeg = -135.0f;
    gApp.editedKnobOptions.endAngleDeg = 135.0f;
    gApp.editedKnobValid = true;

    setEditInt(gApp.knobSize, gApp.editedKnob.preferredCellSize);
    setEditInt(gApp.knobFrames, gApp.editedKnobOptions.frameCount);
    setEditFloat(gApp.knobStartAngle, gApp.editedKnobOptions.startAngleDeg);
    setEditFloat(gApp.knobEndAngle, gApp.editedKnobOptions.endAngleDeg);
    setEditInt(gApp.knobTicks, gApp.editedKnob.tickCount);

    SendMessageW(
        gApp.knobAccentToggle, BM_SETCHECK,
        gApp.editedKnob.drawAccentRing ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(
        gApp.knobTicksToggle, BM_SETCHECK,
        gApp.editedKnob.drawScaleTicks ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(
        gApp.knobKnurlToggle, BM_SETCHECK,
        gApp.editedKnob.drawKnurling ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(
        gApp.knobPointerTipToggle, BM_SETCHECK,
        gApp.editedKnob.drawPointerTip ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(
        gApp.knobBrushedToggle, BM_SETCHECK,
        gApp.editedKnob.drawBrushedBezel ? BST_CHECKED : BST_UNCHECKED, 0);
}

void showKnobEditor(bool visible) {
    const int cmd = visible ? SW_SHOW : SW_HIDE;
    for (HWND h : {
        gApp.knobPanel,
        gApp.knobSize,
        gApp.knobFrames,
        gApp.knobStartAngle,
        gApp.knobEndAngle,
        gApp.knobTicks,
        gApp.knobAccentToggle,
        gApp.knobTicksToggle,
        gApp.knobKnurlToggle,
        gApp.knobPointerTipToggle,
        gApp.knobBrushedToggle,
        gApp.knobBodyColor,
        gApp.knobBezelColor,
        gApp.knobAccentColor,
        gApp.knobPointerColor,
        gApp.knobReset}) {
        ShowWindow(h, cmd);
    }
}

void populatePresets() {
    SendMessageW(gApp.preset, CB_RESETCONTENT, 0, 0);

    const int category = selectedCategory();

    if (category == 0) {
        for (const auto& style : gApp.knobStyles) {
            SendMessageW(
                gApp.preset,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(style.name.c_str()));
        }
    } else if (category == 1) {
        for (const auto& style : gApp.hardwareStyles) {
            SendMessageW(
                gApp.preset,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(style.name.c_str()));
        }
    } else if (category == 2) {
        for (const auto& style : gApp.faderStyles) {
            SendMessageW(
                gApp.preset,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(style.name.c_str()));
        }
    } else {
        for (const auto& style : gApp.meterStyles) {
            SendMessageW(
                gApp.preset,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(style.name.c_str()));
        }
    }

    SendMessageW(gApp.preset, CB_SETCURSEL, 0, 0);

    const bool knobMode = category == 0;
    showKnobEditor(knobMode);
    if (knobMode) {
        syncKnobEditorFromPreset();
    }
}

bool renderSelectedTo(
    const fs::path& outputPath,
    float scaleFactor) {

    const int category = selectedCategory();
    const int preset = selectedPreset();

    if (category == 0) {
        if (preset >= static_cast<int>(gApp.knobStyles.size()) ||
            !gApp.editedKnobValid) {
            return false;
        }

        applyKnobEditor();

        RenderOptions options = gApp.editedKnobOptions;
        options.cellSize = std::max(
            16,
            static_cast<int>(std::lround(
                static_cast<double>(gApp.editedKnob.preferredCellSize) *
                static_cast<double>(scaleFactor))));

        gApp.cellWidth = options.cellSize;
        gApp.cellHeight = options.cellSize;
        gApp.frameCount = options.frameCount;

        KnobRenderer renderer;
        return renderer.renderVerticalFilmstrip(
            gApp.editedKnob, options, outputPath.wstring());
    }

    if (category == 1) {
        if (preset >= static_cast<int>(gApp.hardwareStyles.size())) {
            return false;
        }

        const auto& style = gApp.hardwareStyles[preset];
        const int cellSize = std::max(
            16,
            static_cast<int>(std::lround(
                static_cast<double>(style.preferredCellSize) *
                static_cast<double>(scaleFactor))));

        gApp.cellWidth = cellSize;
        gApp.cellHeight = cellSize;
        gApp.frameCount = style.stateCount;

        HardwareRenderer renderer;
        return renderer.renderVerticalFilmstrip(
            style, cellSize, outputPath.wstring());
    }

    if (category == 2) {
        if (preset >= static_cast<int>(gApp.faderStyles.size())) {
            return false;
        }

        const auto& style = gApp.faderStyles[preset];
        const int width = std::max(
            16,
            static_cast<int>(std::lround(
                static_cast<double>(style.preferredWidth) *
                static_cast<double>(scaleFactor))));
        const int height = std::max(
            32,
            static_cast<int>(std::lround(
                static_cast<double>(style.preferredHeight) *
                static_cast<double>(scaleFactor))));

        gApp.cellWidth = width;
        gApp.cellHeight = height;
        gApp.frameCount = style.frameCount;

        FaderRenderer renderer;
        return renderer.renderVerticalFilmstrip(
            style, width, height, style.frameCount, outputPath.wstring());
    }

    if (preset >= static_cast<int>(gApp.meterStyles.size())) {
        return false;
    }

    const auto& style = gApp.meterStyles[preset];
    const int width = std::max(
        32,
        static_cast<int>(std::lround(
            static_cast<double>(style.preferredWidth) *
            static_cast<double>(scaleFactor))));
    const int height = std::max(
        24,
        static_cast<int>(std::lround(
            static_cast<double>(style.preferredHeight) *
            static_cast<double>(scaleFactor))));

    gApp.cellWidth = width;
    gApp.cellHeight = height;
    gApp.frameCount = style.frameCount;

    MeterRenderer renderer;
    return renderer.renderVerticalFilmstrip(
        style, width, height, style.frameCount, outputPath.wstring());
}

std::wstring selectedStyleName() {
    const int category = selectedCategory();
    const int preset = selectedPreset();

    if (category == 0 && gApp.editedKnobValid) {
        return gApp.editedKnob.name;
    }
    if (category == 1 &&
        preset < static_cast<int>(gApp.hardwareStyles.size())) {
        return gApp.hardwareStyles[preset].name;
    }
    if (category == 2 &&
        preset < static_cast<int>(gApp.faderStyles.size())) {
        return gApp.faderStyles[preset].name;
    }
    if (category == 3 &&
        preset < static_cast<int>(gApp.meterStyles.size())) {
        return gApp.meterStyles[preset].name;
    }
    return L"Asset";
}

bool renderPreview() {
    gApp.previewImage.reset();

    wchar_t tempPath[MAX_PATH] {};
    const DWORD len = GetTempPathW(MAX_PATH, tempPath);
    if (len == 0 || len >= MAX_PATH) {
        setStatus(L"Preview-Pfad konnte nicht erstellt werden.");
        return false;
    }

    gApp.previewPath =
        fs::path(tempPath) / L"125A_GUI_Asset_Designer_preview.png";

    if (!renderSelectedTo(gApp.previewPath, selectedScale())) {
        setStatus(L"Vorschau konnte nicht gerendert werden.");
        return false;
    }

    gApp.previewImage.reset(
        Gdiplus::Image::FromFile(gApp.previewPath.c_str(), FALSE));

    if (!gApp.previewImage ||
        gApp.previewImage->GetLastStatus() != Gdiplus::Ok) {
        gApp.previewImage.reset();
        setStatus(L"Vorschau konnte nicht geladen werden.");
        return false;
    }

    SendMessageW(
        gApp.frame,
        TBM_SETRANGE,
        TRUE,
        MAKELPARAM(0, std::max(0, gApp.frameCount - 1)));

    SendMessageW(
        gApp.frame,
        TBM_SETPOS,
        TRUE,
        std::max(0, (gApp.frameCount - 1) / 2));

    setStatus(
        L"Vorschau: " + selectedStyleName() +
        L" | " + std::to_wstring(gApp.cellWidth) +
        L"x" + std::to_wstring(gApp.cellHeight) +
        L" px | " + std::to_wstring(gApp.frameCount) +
        L" Frames/Zustände");

    InvalidateRect(gApp.window, nullptr, FALSE);
    return true;
}

void exportSelectedSet() {
    const fs::path outputDir =
        fs::current_path() / L"exports-gui";

    std::error_code ec;
    fs::create_directories(outputDir, ec);
    if (ec) {
        MessageBoxW(
            gApp.window,
            L"Export-Ordner konnte nicht angelegt werden.",
            L"125A GUI Asset Designer",
            MB_OK | MB_ICONERROR);
        return;
    }

    const std::vector<float> scales {1.0f, 1.5f, 2.0f, 3.0f};
    const std::wstring baseName =
        L"125A_" + safeFilename(selectedStyleName());

    int failures = 0;

    for (float factor : scales) {
        const int percent =
            static_cast<int>(std::lround(factor * 100.0f));

        const fs::path file =
            outputDir /
            (baseName + L"_" +
             std::to_wstring(percent) +
             L"pct.png");

        if (!renderSelectedTo(file, factor)) {
            ++failures;
        }
    }

    if (failures == 0) {
        setStatus(L"Export fertig: " + outputDir.wstring());
        MessageBoxW(
            gApp.window,
            (L"Vier Auflösungen wurden exportiert nach:\n\n" +
             outputDir.wstring()).c_str(),
            L"125A GUI Asset Designer",
            MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(
            gApp.window,
            L"Mindestens ein Export ist fehlgeschlagen.",
            L"125A GUI Asset Designer",
            MB_OK | MB_ICONERROR);
    }
}

void drawPreview(HDC hdc) {
    RECT client {};
    GetClientRect(gApp.window, &client);

    const int left = 620;
    const int top = 28;
    const int right = client.right - 28;
    const int bottom = client.bottom - 88;

    HBRUSH bg = CreateSolidBrush(RGB(20, 25, 31));
    RECT previewRect {left, top, right, bottom};
    FillRect(hdc, &previewRect, bg);
    DeleteObject(bg);

    HPEN border = CreatePen(PS_SOLID, 1, RGB(69, 78, 88));
    HGDIOBJ oldPen = SelectObject(hdc, border);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, left, top, right, bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(border);

    if (!gApp.previewImage) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(190, 196, 202));
        DrawTextW(
            hdc,
            L"Asset auswählen und Vorschau rendern.",
            -1,
            &previewRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    const int frameIndex = static_cast<int>(
        SendMessageW(gApp.frame, TBM_GETPOS, 0, 0));

    const float srcW = static_cast<float>(gApp.cellWidth);
    const float srcH = static_cast<float>(gApp.cellHeight);
    const float srcY = srcH * static_cast<float>(frameIndex);

    const int areaW = std::max(1, right - left - 60);
    const int areaH = std::max(1, bottom - top - 60);

    const float sx = static_cast<float>(areaW) / srcW;
    const float sy = static_cast<float>(areaH) / srcH;
    const float fit = std::min(sx, sy);

    const int dstW =
        std::max(1, static_cast<int>(std::lround(srcW * fit)));
    const int dstH =
        std::max(1, static_cast<int>(std::lround(srcH * fit)));
    const int dstX = left + (right - left - dstW) / 2;
    const int dstY = top + (bottom - top - dstH) / 2;

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(
        Gdiplus::InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(
        Gdiplus::PixelOffsetModeHighQuality);

    g.DrawImage(
        gApp.previewImage.get(),
        Gdiplus::Rect(dstX, dstY, dstW, dstH),
        0,
        static_cast<INT>(srcY),
        static_cast<INT>(srcW),
        static_cast<INT>(srcH),
        Gdiplus::UnitPixel);
}

HWND makeEdit(
    HWND parent,
    HMENU id,
    int x,
    int y,
    int w,
    int h,
    HFONT font) {

    HWND e = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, w, h,
        parent, id, nullptr, nullptr);

    SendMessageW(e, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return e;
}

HWND makeButton(
    HWND parent,
    HMENU id,
    const wchar_t* text,
    int x,
    int y,
    int w,
    int h,
    DWORD style,
    HFONT font) {

    HWND b = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | style,
        x, y, w, h,
        parent, id, nullptr, nullptr);

    SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return b;
}

void createControls(HWND window) {
    const UINT dpi = GetDpiForWindow(window);
    const int fontHeight = -MulDiv(9, static_cast<int>(dpi), 72);
    HFONT font = CreateFontW(
        fontHeight, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    auto makeStatic = [&](const wchar_t* text, int x, int y, int w, int h) {
        HWND c = CreateWindowExW(
            0, L"STATIC", text,
            WS_CHILD | WS_VISIBLE,
            x, y, w, h,
            window, nullptr, nullptr, nullptr);
        SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return c;
    };

    makeStatic(L"Asset-Typ", 24, 28, 260, 20);
    gApp.category = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        24, 50, 270, 200,
        window,
        reinterpret_cast<HMENU>(IdCategory),
        nullptr, nullptr);

    makeStatic(L"Preset / Ausgangspunkt", 24, 92, 260, 20);
    gApp.preset = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        24, 114, 270, 260,
        window,
        reinterpret_cast<HMENU>(IdPreset),
        nullptr, nullptr);

    makeStatic(L"Vorschau-Auflösung", 24, 156, 260, 20);
    gApp.scale = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        24, 178, 270, 200,
        window,
        reinterpret_cast<HMENU>(IdScale),
        nullptr, nullptr);

    makeStatic(L"Frame / Zustand", 24, 222, 260, 20);
    gApp.frame = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        24, 246, 270, 44,
        window,
        reinterpret_cast<HMENU>(IdFrame),
        nullptr, nullptr);

    HWND renderButton = makeButton(
        window,
        reinterpret_cast<HMENU>(IdRender),
        L"Vorschau rendern",
        24, 306, 270, 36,
        BS_PUSHBUTTON,
        font);

    HWND exportButton = makeButton(
        window,
        reinterpret_cast<HMENU>(IdExport),
        L"4 Auflösungen exportieren",
        24, 350, 270, 36,
        BS_PUSHBUTTON,
        font);

    makeStatic(
        L"Renderer: dieselbe Engine wie CI/GitHub\n"
        L"1x / 1.5x / 2x / 3x\n"
        L"Feste Geometrie / Safe Area / Pixel-Snap",
        24, 410, 270, 72);

    gApp.status = CreateWindowExW(
        0, L"STATIC", L"Bereit.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        24, 510, 270, 90,
        window, nullptr, nullptr, nullptr);

    // Knob editor.
    gApp.knobPanel = makeStatic(
        L"Eigener Knob",
        322, 28, 260, 22);

    makeStatic(L"Größe px", 322, 58, 90, 20);
    gApp.knobSize = makeEdit(
        window, reinterpret_cast<HMENU>(IdSize),
        420, 54, 72, 24, font);

    makeStatic(L"Frames", 322, 88, 90, 20);
    gApp.knobFrames = makeEdit(
        window, reinterpret_cast<HMENU>(IdFrames),
        420, 84, 72, 24, font);

    makeStatic(L"Startwinkel", 322, 118, 90, 20);
    gApp.knobStartAngle = makeEdit(
        window, reinterpret_cast<HMENU>(IdStartAngle),
        420, 114, 72, 24, font);

    makeStatic(L"Endwinkel", 322, 148, 90, 20);
    gApp.knobEndAngle = makeEdit(
        window, reinterpret_cast<HMENU>(IdEndAngle),
        420, 144, 72, 24, font);

    makeStatic(L"Skalenstriche", 322, 178, 90, 20);
    gApp.knobTicks = makeEdit(
        window, reinterpret_cast<HMENU>(IdTicks),
        420, 174, 72, 24, font);

    gApp.knobAccentToggle = makeButton(
        window, reinterpret_cast<HMENU>(IdAccentToggle),
        L"Akzentring",
        322, 214, 125, 24,
        BS_AUTOCHECKBOX, font);

    gApp.knobTicksToggle = makeButton(
        window, reinterpret_cast<HMENU>(IdTicksToggle),
        L"Skala",
        455, 214, 125, 24,
        BS_AUTOCHECKBOX, font);

    gApp.knobKnurlToggle = makeButton(
        window, reinterpret_cast<HMENU>(IdKnurlToggle),
        L"Rändelung",
        322, 242, 125, 24,
        BS_AUTOCHECKBOX, font);

    gApp.knobPointerTipToggle = makeButton(
        window, reinterpret_cast<HMENU>(IdPointerTipToggle),
        L"Pointer-Tip",
        455, 242, 125, 24,
        BS_AUTOCHECKBOX, font);

    gApp.knobBrushedToggle = makeButton(
        window, reinterpret_cast<HMENU>(IdBrushedToggle),
        L"Brushed Bezel",
        322, 270, 258, 24,
        BS_AUTOCHECKBOX, font);

    gApp.knobBodyColor = makeButton(
        window, reinterpret_cast<HMENU>(IdBodyColor),
        L"Körperfarbe...",
        322, 310, 125, 30,
        BS_PUSHBUTTON, font);

    gApp.knobBezelColor = makeButton(
        window, reinterpret_cast<HMENU>(IdBezelColor),
        L"Bezelfarbe...",
        455, 310, 125, 30,
        BS_PUSHBUTTON, font);

    gApp.knobAccentColor = makeButton(
        window, reinterpret_cast<HMENU>(IdAccentColor),
        L"Akzentfarbe...",
        322, 348, 125, 30,
        BS_PUSHBUTTON, font);

    gApp.knobPointerColor = makeButton(
        window, reinterpret_cast<HMENU>(IdPointerColor),
        L"Pointerfarbe...",
        455, 348, 125, 30,
        BS_PUSHBUTTON, font);

    gApp.knobReset = makeButton(
        window, reinterpret_cast<HMENU>(IdResetKnob),
        L"Preset zurücksetzen",
        322, 394, 258, 32,
        BS_PUSHBUTTON, font);

    makeStatic(
        L"Preset wählen → Werte ändern → Vorschau → Export.\n"
        L"Die Änderungen betreffen nur deinen Export, nicht das Originalpreset.",
        322, 448, 258, 62);

    SetPropW(window, L"125A_UI_FONT", font);

    for (HWND c : {
        gApp.category, gApp.preset, gApp.scale, gApp.frame,
        renderButton, exportButton, gApp.status}) {
        SendMessageW(
            c, WM_SETFONT,
            reinterpret_cast<WPARAM>(font), TRUE);
    }

    SendMessageW(
        gApp.category, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"Knobs"));
    SendMessageW(
        gApp.category, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"LEDs / Buttons / Switches"));
    SendMessageW(
        gApp.category, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"Fader"));
    SendMessageW(
        gApp.category, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"Meter / VU"));
    SendMessageW(gApp.category, CB_SETCURSEL, 0, 0);

    SendMessageW(
        gApp.scale, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"1x"));
    SendMessageW(
        gApp.scale, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"1.5x"));
    SendMessageW(
        gApp.scale, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"2x"));
    SendMessageW(
        gApp.scale, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"3x"));
    SendMessageW(gApp.scale, CB_SETCURSEL, 0, 0);

    populatePresets();
}

LRESULT CALLBACK windowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) {

    switch (msg) {
        case WM_CREATE:
            gApp.window = hwnd;
            createControls(hwnd);
            PostMessageW(hwnd, WM_COMMAND, IdRender, 0);
            return 0;

        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            const int code = HIWORD(wParam);

            if (id == IdCategory && code == CBN_SELCHANGE) {
                populatePresets();
                renderPreview();
                return 0;
            }

            if (id == IdPreset && code == CBN_SELCHANGE) {
                if (selectedCategory() == 0) {
                    syncKnobEditorFromPreset();
                }
                renderPreview();
                return 0;
            }

            if (id == IdScale && code == CBN_SELCHANGE) {
                renderPreview();
                return 0;
            }

            if (id == IdResetKnob) {
                syncKnobEditorFromPreset();
                renderPreview();
                return 0;
            }

            if (id == IdBodyColor && gApp.editedKnobValid) {
                if (chooseColor(hwnd, gApp.editedKnob.bodyTop)) {
                    gApp.editedKnob.bodyBottom =
                        darker(gApp.editedKnob.bodyTop, 0.35f);
                    renderPreview();
                }
                return 0;
            }

            if (id == IdBezelColor && gApp.editedKnobValid) {
                if (chooseColor(hwnd, gApp.editedKnob.bezelOuter)) {
                    gApp.editedKnob.bezelInner =
                        darker(gApp.editedKnob.bezelOuter, 0.32f);
                    renderPreview();
                }
                return 0;
            }

            if (id == IdAccentColor && gApp.editedKnobValid) {
                if (chooseColor(hwnd, gApp.editedKnob.accentRing)) {
                    renderPreview();
                }
                return 0;
            }

            if (id == IdPointerColor && gApp.editedKnobValid) {
                if (chooseColor(hwnd, gApp.editedKnob.indicator)) {
                    renderPreview();
                }
                return 0;
            }

            if (id == IdAccentToggle ||
                id == IdTicksToggle ||
                id == IdKnurlToggle ||
                id == IdPointerTipToggle ||
                id == IdBrushedToggle) {
                renderPreview();
                return 0;
            }

            if (id == IdRender) {
                renderPreview();
                return 0;
            }

            if (id == IdExport) {
                exportSelectedSet();
                return 0;
            }
            break;
        }

        case WM_HSCROLL:
            if (reinterpret_cast<HWND>(lParam) == gApp.frame) {
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps {};
            HDC hdc = BeginPaint(hwnd, &ps);

            HBRUSH windowBg = CreateSolidBrush(RGB(30, 36, 43));
            RECT rc {};
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, windowBg);
            DeleteObject(windowBg);

            drawPreview(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            gApp.previewImage.reset();

            if (HFONT font = reinterpret_cast<HFONT>(
                    RemovePropW(hwnd, L"125A_UI_FONT"))) {
                DeleteObject(font);
            }

            if (!gApp.previewPath.empty()) {
                std::error_code ec;
                fs::remove(gApp.previewPath, ec);
            }

            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand) {

    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX controls {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_BAR_CLASSES | ICC_STANDARD_CLASSES
    };
    InitCommonControlsEx(&controls);

    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartupInput gdiplusInput;
    if (Gdiplus::GdiplusStartup(
            &gdiplusToken,
            &gdiplusInput,
            nullptr) != Gdiplus::Ok) {
        MessageBoxW(
            nullptr,
            L"GDI+ konnte nicht initialisiert werden.",
            L"125A GUI Asset Designer",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    gApp.knobStyles = KnobRenderer::builtInStyles();
    gApp.hardwareStyles = HardwareRenderer::builtInStyles();
    gApp.faderStyles = FaderRenderer::builtInStyles();
    gApp.meterStyles = MeterRenderer::builtInStyles();

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClass;

    if (!RegisterClassExW(&wc)) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    const UINT systemDpi = GetDpiForSystem();
    const int initialWidth =
        MulDiv(1420, static_cast<int>(systemDpi), 96);
    const int initialHeight =
        MulDiv(820, static_cast<int>(systemDpi), 96);

    HWND window = CreateWindowExW(
        0,
        kWindowClass,
        L"125A GUI Asset Designer v0.1.0",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        initialWidth, initialHeight,
        nullptr, nullptr, instance, nullptr);

    if (!window) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG msg {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}
