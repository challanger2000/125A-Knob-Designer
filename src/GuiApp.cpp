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
    IdAssetName = 1007,
    IdSavePreset = 1008,
    IdLoadPreset = 1009,

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
    HWND assetName {};
    HWND savePreset {};
    HWND loadPreset {};

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

    HWND fieldLabel1 {};
    HWND fieldLabel2 {};
    HWND fieldLabel3 {};
    HWND fieldLabel4 {};
    HWND fieldLabel5 {};
    HWND editorHint {};

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

    HardwareAssetStyle editedHardware {};
    bool editedHardwareValid {false};

    FaderStyle editedFader {};
    bool editedFaderValid {false};

    MeterStyle editedMeter {};
    bool editedMeterValid {false};
};

AppState gApp;

std::wstring safeFilename(std::wstring name) {
    const std::wstring invalid = L"<>:\"/\\|?*";
    for (auto& ch : name) {
        if (ch == L' ' || invalid.find(ch) != std::wstring::npos ||
            ch < 32) {
            ch = L'_';
        }
    }

    while (!name.empty() &&
           (name.back() == L'.' || name.back() == L' ')) {
        name.pop_back();
    }

    if (name.empty()) {
        name = L"Asset";
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

std::wstring getWindowText(HWND h) {
    const int len = GetWindowTextLengthW(h);
    std::vector<wchar_t> buffer(static_cast<std::size_t>(len + 1), L'\0');
    GetWindowTextW(h, buffer.data(), len + 1);
    return buffer.data();
}

void applyAssetName() {
    const std::wstring name = getWindowText(gApp.assetName);
    if (name.empty()) {
        return;
    }

    switch (selectedCategory()) {
        case 0:
            if (gApp.editedKnobValid) gApp.editedKnob.name = name;
            break;
        case 1:
            if (gApp.editedHardwareValid) gApp.editedHardware.name = name;
            break;
        case 2:
            if (gApp.editedFaderValid) gApp.editedFader.name = name;
            break;
        case 3:
            if (gApp.editedMeterValid) gApp.editedMeter.name = name;
            break;
    }
}

std::wstring colorText(const Color& c) {
    wchar_t buffer[16] {};
    swprintf_s(
        buffer,
        L"%02X%02X%02X%02X",
        static_cast<unsigned>(c.a),
        static_cast<unsigned>(c.r),
        static_cast<unsigned>(c.g),
        static_cast<unsigned>(c.b));
    return buffer;
}

Color parseColor(const std::wstring& text, Color fallback) {
    if (text.size() != 8) {
        return fallback;
    }

    unsigned value = 0;
    if (swscanf_s(text.c_str(), L"%08X", &value) != 1) {
        return fallback;
    }

    return {
        static_cast<std::uint8_t>((value >> 24) & 0xFF),
        static_cast<std::uint8_t>((value >> 16) & 0xFF),
        static_cast<std::uint8_t>((value >> 8) & 0xFF),
        static_cast<std::uint8_t>(value & 0xFF)
    };
}

void iniWrite(
    const std::wstring& path,
    const wchar_t* section,
    const wchar_t* key,
    const std::wstring& value) {
    WritePrivateProfileStringW(section, key, value.c_str(), path.c_str());
}

std::wstring iniRead(
    const std::wstring& path,
    const wchar_t* section,
    const wchar_t* key,
    const std::wstring& fallback = L"") {
    wchar_t buffer[512] {};
    GetPrivateProfileStringW(
        section, key, fallback.c_str(),
        buffer, 512, path.c_str());
    return buffer;
}

int parseIntSetting(
    const std::wstring& value,
    int fallback,
    int minValue,
    int maxValue) {

    try {
        return std::clamp(std::stoi(value), minValue, maxValue);
    } catch (...) {
        return fallback;
    }
}

float parseFloatSetting(
    const std::wstring& value,
    float fallback,
    float minValue,
    float maxValue) {

    try {
        return std::clamp(std::stof(value), minValue, maxValue);
    } catch (...) {
        return fallback;
    }
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
    applyAssetName();
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

void showControl(HWND h, bool visible) {
    if (h) {
        ShowWindow(h, visible ? SW_SHOW : SW_HIDE);
    }
}

void setButtonText(HWND h, const wchar_t* text) {
    if (h) {
        SetWindowTextW(h, text);
    }
}

void syncHardwareEditorFromPreset() {
    const int preset = selectedPreset();
    if (preset < 0 || preset >= static_cast<int>(gApp.hardwareStyles.size())) {
        gApp.editedHardwareValid = false;
        return;
    }

    gApp.editedHardware = gApp.hardwareStyles[preset];
    gApp.editedHardware.name =
        gApp.hardwareStyles[preset].name + L" Custom";
    gApp.editedHardwareValid = true;

    setEditInt(gApp.knobSize, gApp.editedHardware.preferredCellSize);
    setEditInt(gApp.knobFrames, gApp.editedHardware.stateCount);
}

void syncFaderEditorFromPreset() {
    const int preset = selectedPreset();
    if (preset < 0 || preset >= static_cast<int>(gApp.faderStyles.size())) {
        gApp.editedFaderValid = false;
        return;
    }

    gApp.editedFader = gApp.faderStyles[preset];
    gApp.editedFader.name =
        gApp.faderStyles[preset].name + L" Custom";
    gApp.editedFaderValid = true;

    setEditInt(gApp.knobSize, gApp.editedFader.preferredWidth);
    setEditInt(gApp.knobFrames, gApp.editedFader.preferredHeight);
    setEditInt(gApp.knobStartAngle, gApp.editedFader.frameCount);
}

void syncMeterEditorFromPreset() {
    const int preset = selectedPreset();
    if (preset < 0 || preset >= static_cast<int>(gApp.meterStyles.size())) {
        gApp.editedMeterValid = false;
        return;
    }

    gApp.editedMeter = gApp.meterStyles[preset];
    gApp.editedMeter.name =
        gApp.meterStyles[preset].name + L" Custom";
    gApp.editedMeterValid = true;

    setEditInt(gApp.knobSize, gApp.editedMeter.preferredWidth);
    setEditInt(gApp.knobFrames, gApp.editedMeter.preferredHeight);
    setEditInt(gApp.knobStartAngle, gApp.editedMeter.frameCount);
    setEditFloat(gApp.knobEndAngle, gApp.editedMeter.startAngleDeg);
    setEditFloat(gApp.knobTicks, gApp.editedMeter.endAngleDeg);
}

void applyHardwareEditor() {
    if (!gApp.editedHardwareValid) {
        return;
    }

    gApp.editedHardware.preferredCellSize =
        readInt(
            gApp.knobSize,
            gApp.editedHardware.preferredCellSize,
            16,
            512);

    gApp.editedHardware.stateCount =
        readInt(
            gApp.knobFrames,
            gApp.editedHardware.stateCount,
            2,
            16);
    applyAssetName();
}

void applyFaderEditor() {
    if (!gApp.editedFaderValid) {
        return;
    }

    gApp.editedFader.preferredWidth =
        readInt(
            gApp.knobSize,
            gApp.editedFader.preferredWidth,
            24,
            512);

    gApp.editedFader.preferredHeight =
        readInt(
            gApp.knobFrames,
            gApp.editedFader.preferredHeight,
            48,
            1024);

    gApp.editedFader.frameCount =
        readInt(
            gApp.knobStartAngle,
            gApp.editedFader.frameCount,
            2,
            256);
    applyAssetName();
}

void applyMeterEditor() {
    if (!gApp.editedMeterValid) {
        return;
    }

    gApp.editedMeter.preferredWidth =
        readInt(
            gApp.knobSize,
            gApp.editedMeter.preferredWidth,
            48,
            1024);

    gApp.editedMeter.preferredHeight =
        readInt(
            gApp.knobFrames,
            gApp.editedMeter.preferredHeight,
            32,
            768);

    gApp.editedMeter.frameCount =
        readInt(
            gApp.knobStartAngle,
            gApp.editedMeter.frameCount,
            2,
            256);

    gApp.editedMeter.startAngleDeg =
        readFloat(
            gApp.knobEndAngle,
            gApp.editedMeter.startAngleDeg,
            -180.0f,
            180.0f);

    gApp.editedMeter.endAngleDeg =
        readFloat(
            gApp.knobTicks,
            gApp.editedMeter.endAngleDeg,
            -180.0f,
            180.0f);
    applyAssetName();
}

void configureEditorForCategory() {
    const int category = selectedCategory();
    const bool knob = category == 0;
    const bool hardware = category == 1;
    const bool fader = category == 2;
    const bool meter = category == 3;

    showControl(gApp.knobPanel, true);
    setButtonText(
        gApp.knobPanel,
        knob ? L"Eigener Knob" :
        hardware ? L"Eigene Hardware" :
        fader ? L"Eigener Fader" :
        L"Eigenes VU / Meter");

    showControl(gApp.fieldLabel1, true);
    showControl(gApp.fieldLabel2, true);
    showControl(gApp.fieldLabel3, knob || fader || meter);
    showControl(gApp.fieldLabel4, knob || meter);
    showControl(gApp.fieldLabel5, knob || meter);

    showControl(gApp.knobSize, true);
    showControl(gApp.knobFrames, true);
    showControl(gApp.knobStartAngle, knob || fader || meter);
    showControl(gApp.knobEndAngle, knob || meter);
    showControl(gApp.knobTicks, knob || meter);

    showControl(gApp.knobAccentToggle, knob);
    showControl(gApp.knobTicksToggle, knob);
    showControl(gApp.knobKnurlToggle, knob);
    showControl(gApp.knobPointerTipToggle, knob);
    showControl(gApp.knobBrushedToggle, knob);

    showControl(gApp.knobBodyColor, true);
    showControl(gApp.knobBezelColor, true);
    showControl(gApp.knobAccentColor, true);
    showControl(gApp.knobPointerColor, true);
    showControl(gApp.knobReset, true);
    showControl(gApp.editorHint, true);

    if (knob) {
        SetWindowTextW(gApp.fieldLabel1, L"Größe px");
        SetWindowTextW(gApp.fieldLabel2, L"Frames");
        SetWindowTextW(gApp.fieldLabel3, L"Startwinkel");
        SetWindowTextW(gApp.fieldLabel4, L"Endwinkel");
        SetWindowTextW(gApp.fieldLabel5, L"Skalenstriche");
        setButtonText(gApp.knobBodyColor, L"Körperfarbe...");
        setButtonText(gApp.knobBezelColor, L"Bezelfarbe...");
        setButtonText(gApp.knobAccentColor, L"Akzentfarbe...");
        setButtonText(gApp.knobPointerColor, L"Pointerfarbe...");
        SetWindowTextW(
            gApp.editorHint,
            L"Preset wählen → Werte ändern → Vorschau → Export.\n"
            L"Das Originalpreset bleibt unverändert.");
        syncKnobEditorFromPreset();
        SetWindowTextW(gApp.assetName, gApp.editedKnob.name.c_str());
    } else if (hardware) {
        SetWindowTextW(gApp.fieldLabel1, L"Größe px");
        SetWindowTextW(gApp.fieldLabel2, L"Zustände");
        setButtonText(gApp.knobBodyColor, L"Face-Farbe...");
        setButtonText(gApp.knobBezelColor, L"Metallfarbe...");
        setButtonText(gApp.knobAccentColor, L"ON / Akzent...");
        setButtonText(gApp.knobPointerColor, L"OFF / Dunkel...");
        SetWindowTextW(
            gApp.editorHint,
            L"LED, Button oder Switch als Ausgangspunkt wählen.\n"
            L"Größe, Zustände und Materialfarben sind editierbar.");
        syncHardwareEditorFromPreset();
        SetWindowTextW(gApp.assetName, gApp.editedHardware.name.c_str());
    } else if (fader) {
        SetWindowTextW(gApp.fieldLabel1, L"Breite px");
        SetWindowTextW(gApp.fieldLabel2, L"Höhe px");
        SetWindowTextW(gApp.fieldLabel3, L"Frames");
        setButtonText(gApp.knobBodyColor, L"Cap-Farbe...");
        setButtonText(gApp.knobBezelColor, L"Rail-Farbe...");
        setButtonText(gApp.knobAccentColor, L"Pointerfarbe...");
        setButtonText(gApp.knobPointerColor, L"Tick-Farbe...");
        SetWindowTextW(
            gApp.editorHint,
            L"Fader-Geometrie und Materialfarben editieren.\n"
            L"Der Export bleibt VSTGUI-filmstrip-tauglich.");
        syncFaderEditorFromPreset();
        SetWindowTextW(gApp.assetName, gApp.editedFader.name.c_str());
    } else if (meter) {
        SetWindowTextW(gApp.fieldLabel1, L"Breite px");
        SetWindowTextW(gApp.fieldLabel2, L"Höhe px");
        SetWindowTextW(gApp.fieldLabel3, L"Frames");
        SetWindowTextW(gApp.fieldLabel4, L"Startwinkel");
        SetWindowTextW(gApp.fieldLabel5, L"Endwinkel");
        setButtonText(gApp.knobBodyColor, L"Face-Farbe...");
        setButtonText(gApp.knobBezelColor, L"Frame-Farbe...");
        setButtonText(gApp.knobAccentColor, L"Nadelfarbe...");
        setButtonText(gApp.knobPointerColor, L"Red-Zone...");
        SetWindowTextW(
            gApp.editorHint,
            L"Meter-Abmessungen, Winkel und Farben editieren.\n"
            L"Die Nadel wird als deterministischer Filmstrip erzeugt.");
        syncMeterEditorFromPreset();
        SetWindowTextW(gApp.assetName, gApp.editedMeter.name.c_str());
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

    configureEditorForCategory();
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
        if (preset >= static_cast<int>(gApp.hardwareStyles.size()) ||
            !gApp.editedHardwareValid) {
            return false;
        }

        applyHardwareEditor();

        const int cellSize = std::max(
            16,
            static_cast<int>(std::lround(
                static_cast<double>(gApp.editedHardware.preferredCellSize) *
                static_cast<double>(scaleFactor))));

        gApp.cellWidth = cellSize;
        gApp.cellHeight = cellSize;
        gApp.frameCount = gApp.editedHardware.stateCount;

        HardwareRenderer renderer;
        return renderer.renderVerticalFilmstrip(
            gApp.editedHardware, cellSize, outputPath.wstring());
    }

    if (category == 2) {
        if (preset >= static_cast<int>(gApp.faderStyles.size()) ||
            !gApp.editedFaderValid) {
            return false;
        }

        applyFaderEditor();

        const int width = std::max(
            16,
            static_cast<int>(std::lround(
                static_cast<double>(gApp.editedFader.preferredWidth) *
                static_cast<double>(scaleFactor))));
        const int height = std::max(
            32,
            static_cast<int>(std::lround(
                static_cast<double>(gApp.editedFader.preferredHeight) *
                static_cast<double>(scaleFactor))));

        gApp.cellWidth = width;
        gApp.cellHeight = height;
        gApp.frameCount = gApp.editedFader.frameCount;

        FaderRenderer renderer;
        return renderer.renderVerticalFilmstrip(
            gApp.editedFader,
            width,
            height,
            gApp.editedFader.frameCount,
            outputPath.wstring());
    }

    if (preset >= static_cast<int>(gApp.meterStyles.size()) ||
        !gApp.editedMeterValid) {
        return false;
    }

    applyMeterEditor();

    const int width = std::max(
        32,
        static_cast<int>(std::lround(
            static_cast<double>(gApp.editedMeter.preferredWidth) *
            static_cast<double>(scaleFactor))));
    const int height = std::max(
        24,
        static_cast<int>(std::lround(
            static_cast<double>(gApp.editedMeter.preferredHeight) *
            static_cast<double>(scaleFactor))));

    gApp.cellWidth = width;
    gApp.cellHeight = height;
    gApp.frameCount = gApp.editedMeter.frameCount;

    MeterRenderer renderer;
    return renderer.renderVerticalFilmstrip(
        gApp.editedMeter,
        width,
        height,
        gApp.editedMeter.frameCount,
        outputPath.wstring());
}

std::wstring selectedStyleName() {
    const int category = selectedCategory();
    const int preset = selectedPreset();

    if (category == 0 && gApp.editedKnobValid) {
        return gApp.editedKnob.name;
    }
    if (category == 1 && gApp.editedHardwareValid) {
        return gApp.editedHardware.name;
    }
    if (category == 2 && gApp.editedFaderValid) {
        return gApp.editedFader.name;
    }
    if (category == 3 && gApp.editedMeterValid) {
        return gApp.editedMeter.name;
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

bool choosePresetPath(HWND owner, bool save, std::wstring& path) {
    wchar_t file[MAX_PATH] {};
    OPENFILENAMEW ofn {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter =
        L"125A Preset (*.125apreset)\0*.125apreset\0"
        L"Alle Dateien (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"125apreset";
    ofn.Flags = save
        ? OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST
        : OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    const BOOL ok = save
        ? GetSaveFileNameW(&ofn)
        : GetOpenFileNameW(&ofn);

    if (!ok) {
        return false;
    }

    path = file;
    return true;
}

void saveCustomPreset() {
    std::wstring path;
    if (!choosePresetPath(gApp.window, true, path)) {
        return;
    }

    applyAssetName();
    const int category = selectedCategory();
    const int basePreset = selectedPreset();

    iniWrite(path, L"Preset", L"Version", L"1");
    iniWrite(path, L"Preset", L"Category", std::to_wstring(category));
    iniWrite(path, L"Preset", L"BasePreset", std::to_wstring(basePreset));
    iniWrite(path, L"Preset", L"Name", getWindowText(gApp.assetName));

    if (category == 0 && gApp.editedKnobValid) {
        applyKnobEditor();
        iniWrite(path, L"Knob", L"Size", std::to_wstring(gApp.editedKnob.preferredCellSize));
        iniWrite(path, L"Knob", L"Frames", std::to_wstring(gApp.editedKnobOptions.frameCount));
        iniWrite(path, L"Knob", L"StartAngle", std::to_wstring(gApp.editedKnobOptions.startAngleDeg));
        iniWrite(path, L"Knob", L"EndAngle", std::to_wstring(gApp.editedKnobOptions.endAngleDeg));
        iniWrite(path, L"Knob", L"Ticks", std::to_wstring(gApp.editedKnob.tickCount));
        iniWrite(path, L"Knob", L"AccentRing", gApp.editedKnob.drawAccentRing ? L"1" : L"0");
        iniWrite(path, L"Knob", L"ScaleTicks", gApp.editedKnob.drawScaleTicks ? L"1" : L"0");
        iniWrite(path, L"Knob", L"Knurling", gApp.editedKnob.drawKnurling ? L"1" : L"0");
        iniWrite(path, L"Knob", L"PointerTip", gApp.editedKnob.drawPointerTip ? L"1" : L"0");
        iniWrite(path, L"Knob", L"BrushedBezel", gApp.editedKnob.drawBrushedBezel ? L"1" : L"0");
        iniWrite(path, L"Knob", L"BodyTop", colorText(gApp.editedKnob.bodyTop));
        iniWrite(path, L"Knob", L"BodyBottom", colorText(gApp.editedKnob.bodyBottom));
        iniWrite(path, L"Knob", L"BezelOuter", colorText(gApp.editedKnob.bezelOuter));
        iniWrite(path, L"Knob", L"BezelInner", colorText(gApp.editedKnob.bezelInner));
        iniWrite(path, L"Knob", L"Accent", colorText(gApp.editedKnob.accentRing));
        iniWrite(path, L"Knob", L"Indicator", colorText(gApp.editedKnob.indicator));
    } else if (category == 1 && gApp.editedHardwareValid) {
        applyHardwareEditor();
        iniWrite(path, L"Hardware", L"Kind", std::to_wstring(static_cast<int>(gApp.editedHardware.kind)));
        iniWrite(path, L"Hardware", L"Size", std::to_wstring(gApp.editedHardware.preferredCellSize));
        iniWrite(path, L"Hardware", L"States", std::to_wstring(gApp.editedHardware.stateCount));
        iniWrite(path, L"Hardware", L"FaceTop", colorText(gApp.editedHardware.faceTop));
        iniWrite(path, L"Hardware", L"FaceBottom", colorText(gApp.editedHardware.faceBottom));
        iniWrite(path, L"Hardware", L"MetalTop", colorText(gApp.editedHardware.metalTop));
        iniWrite(path, L"Hardware", L"MetalBottom", colorText(gApp.editedHardware.metalBottom));
        iniWrite(path, L"Hardware", L"AccentOn", colorText(gApp.editedHardware.accentOn));
        iniWrite(path, L"Hardware", L"AccentOff", colorText(gApp.editedHardware.accentOff));
    } else if (category == 2 && gApp.editedFaderValid) {
        applyFaderEditor();
        iniWrite(path, L"Fader", L"Width", std::to_wstring(gApp.editedFader.preferredWidth));
        iniWrite(path, L"Fader", L"Height", std::to_wstring(gApp.editedFader.preferredHeight));
        iniWrite(path, L"Fader", L"Frames", std::to_wstring(gApp.editedFader.frameCount));
        iniWrite(path, L"Fader", L"CapTop", colorText(gApp.editedFader.capTop));
        iniWrite(path, L"Fader", L"CapBottom", colorText(gApp.editedFader.capBottom));
        iniWrite(path, L"Fader", L"RailInner", colorText(gApp.editedFader.railInner));
        iniWrite(path, L"Fader", L"RailOuter", colorText(gApp.editedFader.railOuter));
        iniWrite(path, L"Fader", L"Indicator", colorText(gApp.editedFader.indicator));
        iniWrite(path, L"Fader", L"Tick", colorText(gApp.editedFader.tick));
    } else if (category == 3 && gApp.editedMeterValid) {
        applyMeterEditor();
        iniWrite(path, L"Meter", L"Width", std::to_wstring(gApp.editedMeter.preferredWidth));
        iniWrite(path, L"Meter", L"Height", std::to_wstring(gApp.editedMeter.preferredHeight));
        iniWrite(path, L"Meter", L"Frames", std::to_wstring(gApp.editedMeter.frameCount));
        iniWrite(path, L"Meter", L"StartAngle", std::to_wstring(gApp.editedMeter.startAngleDeg));
        iniWrite(path, L"Meter", L"EndAngle", std::to_wstring(gApp.editedMeter.endAngleDeg));
        iniWrite(path, L"Meter", L"FaceTop", colorText(gApp.editedMeter.faceTop));
        iniWrite(path, L"Meter", L"FaceBottom", colorText(gApp.editedMeter.faceBottom));
        iniWrite(path, L"Meter", L"FrameTop", colorText(gApp.editedMeter.frameTop));
        iniWrite(path, L"Meter", L"FrameBottom", colorText(gApp.editedMeter.frameBottom));
        iniWrite(path, L"Meter", L"Needle", colorText(gApp.editedMeter.needle));
        iniWrite(path, L"Meter", L"RedZone", colorText(gApp.editedMeter.redZone));
    }

    setStatus(L"Preset gespeichert: " + path);
}

void loadCustomPreset() {
    std::wstring path;
    if (!choosePresetPath(gApp.window, false, path)) {
        return;
    }

    const int category =
        parseIntSetting(iniRead(path, L"Preset", L"Category", L"0"), 0, 0, 3);

    SendMessageW(gApp.category, CB_SETCURSEL, category, 0);
    populatePresets();

    const int count = static_cast<int>(
        SendMessageW(gApp.preset, CB_GETCOUNT, 0, 0));
    const int basePreset =
        parseIntSetting(
            iniRead(path, L"Preset", L"BasePreset", L"0"),
            0,
            0,
            std::max(0, count - 1));

    SendMessageW(gApp.preset, CB_SETCURSEL, basePreset, 0);
    configureEditorForCategory();

    const std::wstring name =
        iniRead(path, L"Preset", L"Name", L"Custom Asset");
    SetWindowTextW(gApp.assetName, name.c_str());

    if (category == 0 && gApp.editedKnobValid) {
        gApp.editedKnob.name = name;
        gApp.editedKnob.preferredCellSize =
            parseIntSetting(iniRead(path, L"Knob", L"Size", std::to_wstring(gApp.editedKnob.preferredCellSize)), gApp.editedKnob.preferredCellSize, 24, 512);
        gApp.editedKnobOptions.frameCount =
            parseIntSetting(iniRead(path, L"Knob", L"Frames", std::to_wstring(gApp.editedKnobOptions.frameCount)), gApp.editedKnobOptions.frameCount, 2, 256);
        gApp.editedKnobOptions.startAngleDeg =
            parseFloatSetting(iniRead(path, L"Knob", L"StartAngle", std::to_wstring(gApp.editedKnobOptions.startAngleDeg)), gApp.editedKnobOptions.startAngleDeg, -360.0f, 360.0f);
        gApp.editedKnobOptions.endAngleDeg =
            parseFloatSetting(iniRead(path, L"Knob", L"EndAngle", std::to_wstring(gApp.editedKnobOptions.endAngleDeg)), gApp.editedKnobOptions.endAngleDeg, -360.0f, 360.0f);
        gApp.editedKnob.tickCount =
            parseIntSetting(iniRead(path, L"Knob", L"Ticks", std::to_wstring(gApp.editedKnob.tickCount)), gApp.editedKnob.tickCount, 3, 41);
        gApp.editedKnob.drawAccentRing = iniRead(path, L"Knob", L"AccentRing", L"0") == L"1";
        gApp.editedKnob.drawScaleTicks = iniRead(path, L"Knob", L"ScaleTicks", L"0") == L"1";
        gApp.editedKnob.drawKnurling = iniRead(path, L"Knob", L"Knurling", L"0") == L"1";
        gApp.editedKnob.drawPointerTip = iniRead(path, L"Knob", L"PointerTip", L"0") == L"1";
        gApp.editedKnob.drawBrushedBezel = iniRead(path, L"Knob", L"BrushedBezel", L"0") == L"1";
        gApp.editedKnob.bodyTop = parseColor(iniRead(path, L"Knob", L"BodyTop"), gApp.editedKnob.bodyTop);
        gApp.editedKnob.bodyBottom = parseColor(iniRead(path, L"Knob", L"BodyBottom"), gApp.editedKnob.bodyBottom);
        gApp.editedKnob.bezelOuter = parseColor(iniRead(path, L"Knob", L"BezelOuter"), gApp.editedKnob.bezelOuter);
        gApp.editedKnob.bezelInner = parseColor(iniRead(path, L"Knob", L"BezelInner"), gApp.editedKnob.bezelInner);
        gApp.editedKnob.accentRing = parseColor(iniRead(path, L"Knob", L"Accent"), gApp.editedKnob.accentRing);
        gApp.editedKnob.indicator = parseColor(iniRead(path, L"Knob", L"Indicator"), gApp.editedKnob.indicator);
        setEditInt(gApp.knobSize, gApp.editedKnob.preferredCellSize);
        setEditInt(gApp.knobFrames, gApp.editedKnobOptions.frameCount);
        setEditFloat(gApp.knobStartAngle, gApp.editedKnobOptions.startAngleDeg);
        setEditFloat(gApp.knobEndAngle, gApp.editedKnobOptions.endAngleDeg);
        setEditInt(gApp.knobTicks, gApp.editedKnob.tickCount);
        SendMessageW(gApp.knobAccentToggle, BM_SETCHECK, gApp.editedKnob.drawAccentRing ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(gApp.knobTicksToggle, BM_SETCHECK, gApp.editedKnob.drawScaleTicks ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(gApp.knobKnurlToggle, BM_SETCHECK, gApp.editedKnob.drawKnurling ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(gApp.knobPointerTipToggle, BM_SETCHECK, gApp.editedKnob.drawPointerTip ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(gApp.knobBrushedToggle, BM_SETCHECK, gApp.editedKnob.drawBrushedBezel ? BST_CHECKED : BST_UNCHECKED, 0);
    } else if (category == 1 && gApp.editedHardwareValid) {
        gApp.editedHardware.name = name;
        gApp.editedHardware.preferredCellSize =
            parseIntSetting(iniRead(path, L"Hardware", L"Size", std::to_wstring(gApp.editedHardware.preferredCellSize)), gApp.editedHardware.preferredCellSize, 16, 512);
        gApp.editedHardware.stateCount =
            parseIntSetting(iniRead(path, L"Hardware", L"States", std::to_wstring(gApp.editedHardware.stateCount)), gApp.editedHardware.stateCount, 2, 16);
        gApp.editedHardware.kind = static_cast<HardwareAssetKind>(
            parseIntSetting(iniRead(path, L"Hardware", L"Kind", std::to_wstring(static_cast<int>(gApp.editedHardware.kind))), static_cast<int>(gApp.editedHardware.kind), 0, 3));
        gApp.editedHardware.faceTop = parseColor(iniRead(path, L"Hardware", L"FaceTop"), gApp.editedHardware.faceTop);
        gApp.editedHardware.faceBottom = parseColor(iniRead(path, L"Hardware", L"FaceBottom"), gApp.editedHardware.faceBottom);
        gApp.editedHardware.metalTop = parseColor(iniRead(path, L"Hardware", L"MetalTop"), gApp.editedHardware.metalTop);
        gApp.editedHardware.metalBottom = parseColor(iniRead(path, L"Hardware", L"MetalBottom"), gApp.editedHardware.metalBottom);
        gApp.editedHardware.accentOn = parseColor(iniRead(path, L"Hardware", L"AccentOn"), gApp.editedHardware.accentOn);
        gApp.editedHardware.accentOff = parseColor(iniRead(path, L"Hardware", L"AccentOff"), gApp.editedHardware.accentOff);
        setEditInt(gApp.knobSize, gApp.editedHardware.preferredCellSize);
        setEditInt(gApp.knobFrames, gApp.editedHardware.stateCount);
    } else if (category == 2 && gApp.editedFaderValid) {
        gApp.editedFader.name = name;
        gApp.editedFader.preferredWidth =
            parseIntSetting(iniRead(path, L"Fader", L"Width", std::to_wstring(gApp.editedFader.preferredWidth)), gApp.editedFader.preferredWidth, 24, 512);
        gApp.editedFader.preferredHeight =
            parseIntSetting(iniRead(path, L"Fader", L"Height", std::to_wstring(gApp.editedFader.preferredHeight)), gApp.editedFader.preferredHeight, 48, 1024);
        gApp.editedFader.frameCount =
            parseIntSetting(iniRead(path, L"Fader", L"Frames", std::to_wstring(gApp.editedFader.frameCount)), gApp.editedFader.frameCount, 2, 256);
        gApp.editedFader.capTop = parseColor(iniRead(path, L"Fader", L"CapTop"), gApp.editedFader.capTop);
        gApp.editedFader.capBottom = parseColor(iniRead(path, L"Fader", L"CapBottom"), gApp.editedFader.capBottom);
        gApp.editedFader.railInner = parseColor(iniRead(path, L"Fader", L"RailInner"), gApp.editedFader.railInner);
        gApp.editedFader.railOuter = parseColor(iniRead(path, L"Fader", L"RailOuter"), gApp.editedFader.railOuter);
        gApp.editedFader.indicator = parseColor(iniRead(path, L"Fader", L"Indicator"), gApp.editedFader.indicator);
        gApp.editedFader.tick = parseColor(iniRead(path, L"Fader", L"Tick"), gApp.editedFader.tick);
        setEditInt(gApp.knobSize, gApp.editedFader.preferredWidth);
        setEditInt(gApp.knobFrames, gApp.editedFader.preferredHeight);
        setEditInt(gApp.knobStartAngle, gApp.editedFader.frameCount);
    } else if (category == 3 && gApp.editedMeterValid) {
        gApp.editedMeter.name = name;
        gApp.editedMeter.preferredWidth =
            parseIntSetting(iniRead(path, L"Meter", L"Width", std::to_wstring(gApp.editedMeter.preferredWidth)), gApp.editedMeter.preferredWidth, 48, 1024);
        gApp.editedMeter.preferredHeight =
            parseIntSetting(iniRead(path, L"Meter", L"Height", std::to_wstring(gApp.editedMeter.preferredHeight)), gApp.editedMeter.preferredHeight, 32, 768);
        gApp.editedMeter.frameCount =
            parseIntSetting(iniRead(path, L"Meter", L"Frames", std::to_wstring(gApp.editedMeter.frameCount)), gApp.editedMeter.frameCount, 2, 256);
        gApp.editedMeter.startAngleDeg =
            parseFloatSetting(iniRead(path, L"Meter", L"StartAngle", std::to_wstring(gApp.editedMeter.startAngleDeg)), gApp.editedMeter.startAngleDeg, -180.0f, 180.0f);
        gApp.editedMeter.endAngleDeg =
            parseFloatSetting(iniRead(path, L"Meter", L"EndAngle", std::to_wstring(gApp.editedMeter.endAngleDeg)), gApp.editedMeter.endAngleDeg, -180.0f, 180.0f);
        gApp.editedMeter.faceTop = parseColor(iniRead(path, L"Meter", L"FaceTop"), gApp.editedMeter.faceTop);
        gApp.editedMeter.faceBottom = parseColor(iniRead(path, L"Meter", L"FaceBottom"), gApp.editedMeter.faceBottom);
        gApp.editedMeter.frameTop = parseColor(iniRead(path, L"Meter", L"FrameTop"), gApp.editedMeter.frameTop);
        gApp.editedMeter.frameBottom = parseColor(iniRead(path, L"Meter", L"FrameBottom"), gApp.editedMeter.frameBottom);
        gApp.editedMeter.needle = parseColor(iniRead(path, L"Meter", L"Needle"), gApp.editedMeter.needle);
        gApp.editedMeter.redZone = parseColor(iniRead(path, L"Meter", L"RedZone"), gApp.editedMeter.redZone);
        setEditInt(gApp.knobSize, gApp.editedMeter.preferredWidth);
        setEditInt(gApp.knobFrames, gApp.editedMeter.preferredHeight);
        setEditInt(gApp.knobStartAngle, gApp.editedMeter.frameCount);
        setEditFloat(gApp.knobEndAngle, gApp.editedMeter.startAngleDeg);
        setEditFloat(gApp.knobTicks, gApp.editedMeter.endAngleDeg);
    }

    renderPreview();
    setStatus(L"Preset geladen: " + path);
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

        const fs::path tempFile =
            outputDir /
            (baseName + L"_" +
             std::to_wstring(percent) +
             L"pct_tmp.png");

        if (!renderSelectedTo(tempFile, factor)) {
            ++failures;
            continue;
        }

        const fs::path finalFile =
            outputDir /
            (baseName + L"_" +
             std::to_wstring(gApp.cellWidth) + L"x" +
             std::to_wstring(gApp.cellHeight) + L"px_" +
             std::to_wstring(percent) + L"pct_" +
             std::to_wstring(gApp.frameCount) + L"f.png");

        std::error_code renameEc;
        fs::remove(finalFile, renameEc);
        renameEc.clear();
        fs::rename(tempFile, finalFile, renameEc);
        if (renameEc) {
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

    makeStatic(L"Asset-Name", 24, 296, 260, 20);
    gApp.assetName = makeEdit(
        window,
        reinterpret_cast<HMENU>(IdAssetName),
        24, 318, 270, 26, font);

    gApp.savePreset = makeButton(
        window,
        reinterpret_cast<HMENU>(IdSavePreset),
        L"Preset speichern...",
        24, 354, 130, 32,
        BS_PUSHBUTTON,
        font);

    gApp.loadPreset = makeButton(
        window,
        reinterpret_cast<HMENU>(IdLoadPreset),
        L"Preset laden...",
        164, 354, 130, 32,
        BS_PUSHBUTTON,
        font);

    HWND renderButton = makeButton(
        window,
        reinterpret_cast<HMENU>(IdRender),
        L"Vorschau rendern",
        24, 398, 270, 34,
        BS_PUSHBUTTON,
        font);

    HWND exportButton = makeButton(
        window,
        reinterpret_cast<HMENU>(IdExport),
        L"4 Auflösungen exportieren",
        24, 440, 270, 34,
        BS_PUSHBUTTON,
        font);

    makeStatic(
        L"Renderer: dieselbe Engine wie CI/GitHub\n"
        L"1x / 1.5x / 2x / 3x\n"
        L"Feste Geometrie / Safe Area / Pixel-Snap",
        24, 490, 270, 62);

    gApp.status = CreateWindowExW(
        0, L"STATIC", L"Bereit.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        24, 570, 270, 72,
        window, nullptr, nullptr, nullptr);

    // Knob editor.
    gApp.knobPanel = makeStatic(
        L"Eigener Knob",
        322, 28, 260, 22);

    gApp.fieldLabel1 = makeStatic(L"Größe px", 322, 58, 90, 20);
    gApp.knobSize = makeEdit(
        window, reinterpret_cast<HMENU>(IdSize),
        420, 54, 72, 24, font);

    gApp.fieldLabel2 = makeStatic(L"Frames", 322, 88, 90, 20);
    gApp.knobFrames = makeEdit(
        window, reinterpret_cast<HMENU>(IdFrames),
        420, 84, 72, 24, font);

    gApp.fieldLabel3 = makeStatic(L"Startwinkel", 322, 118, 90, 20);
    gApp.knobStartAngle = makeEdit(
        window, reinterpret_cast<HMENU>(IdStartAngle),
        420, 114, 72, 24, font);

    gApp.fieldLabel4 = makeStatic(L"Endwinkel", 322, 148, 90, 20);
    gApp.knobEndAngle = makeEdit(
        window, reinterpret_cast<HMENU>(IdEndAngle),
        420, 144, 72, 24, font);

    gApp.fieldLabel5 = makeStatic(L"Skalenstriche", 322, 178, 90, 20);
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

    gApp.editorHint = makeStatic(
        L"Preset wählen → Werte ändern → Vorschau → Export.\n"
        L"Das Originalpreset bleibt unverändert.",
        322, 448, 258, 62);

    SetPropW(window, L"125A_UI_FONT", font);

    for (HWND c : {
        gApp.category, gApp.preset, gApp.scale, gApp.frame,
        gApp.assetName, gApp.savePreset, gApp.loadPreset,
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
                configureEditorForCategory();
                renderPreview();
                return 0;
            }

            if (id == IdScale && code == CBN_SELCHANGE) {
                renderPreview();
                return 0;
            }

            if (id == IdSavePreset) {
                saveCustomPreset();
                return 0;
            }

            if (id == IdLoadPreset) {
                loadCustomPreset();
                return 0;
            }

            if ((id == IdAssetName ||
                 id == IdSize ||
                 id == IdFrames ||
                 id == IdStartAngle ||
                 id == IdEndAngle ||
                 id == IdTicks) &&
                code == EN_KILLFOCUS) {
                renderPreview();
                return 0;
            }

            if (id == IdResetKnob) {
                configureEditorForCategory();
                renderPreview();
                return 0;
            }

            if (id == IdBodyColor) {
                const int category = selectedCategory();
                bool changed = false;

                if (category == 0 && gApp.editedKnobValid) {
                    changed = chooseColor(hwnd, gApp.editedKnob.bodyTop);
                    if (changed) {
                        gApp.editedKnob.bodyBottom =
                            darker(gApp.editedKnob.bodyTop, 0.35f);
                    }
                } else if (category == 1 && gApp.editedHardwareValid) {
                    changed = chooseColor(hwnd, gApp.editedHardware.faceTop);
                    if (changed) {
                        gApp.editedHardware.faceBottom =
                            darker(gApp.editedHardware.faceTop, 0.32f);
                    }
                } else if (category == 2 && gApp.editedFaderValid) {
                    changed = chooseColor(hwnd, gApp.editedFader.capTop);
                    if (changed) {
                        gApp.editedFader.capBottom =
                            darker(gApp.editedFader.capTop, 0.28f);
                    }
                } else if (category == 3 && gApp.editedMeterValid) {
                    changed = chooseColor(hwnd, gApp.editedMeter.faceTop);
                    if (changed) {
                        gApp.editedMeter.faceBottom =
                            darker(gApp.editedMeter.faceTop, 0.78f);
                    }
                }

                if (changed) {
                    renderPreview();
                }
                return 0;
            }

            if (id == IdBezelColor) {
                const int category = selectedCategory();
                bool changed = false;

                if (category == 0 && gApp.editedKnobValid) {
                    changed = chooseColor(hwnd, gApp.editedKnob.bezelOuter);
                    if (changed) {
                        gApp.editedKnob.bezelInner =
                            darker(gApp.editedKnob.bezelOuter, 0.32f);
                    }
                } else if (category == 1 && gApp.editedHardwareValid) {
                    changed = chooseColor(hwnd, gApp.editedHardware.metalTop);
                    if (changed) {
                        gApp.editedHardware.metalBottom =
                            darker(gApp.editedHardware.metalTop, 0.30f);
                    }
                } else if (category == 2 && gApp.editedFaderValid) {
                    changed = chooseColor(hwnd, gApp.editedFader.railInner);
                    if (changed) {
                        gApp.editedFader.railOuter =
                            darker(gApp.editedFader.railInner, 0.28f);
                    }
                } else if (category == 3 && gApp.editedMeterValid) {
                    changed = chooseColor(hwnd, gApp.editedMeter.frameTop);
                    if (changed) {
                        gApp.editedMeter.frameBottom =
                            darker(gApp.editedMeter.frameTop, 0.28f);
                    }
                }

                if (changed) {
                    renderPreview();
                }
                return 0;
            }

            if (id == IdAccentColor) {
                const int category = selectedCategory();
                bool changed = false;

                if (category == 0 && gApp.editedKnobValid) {
                    changed = chooseColor(hwnd, gApp.editedKnob.accentRing);
                } else if (category == 1 && gApp.editedHardwareValid) {
                    changed = chooseColor(hwnd, gApp.editedHardware.accentOn);
                } else if (category == 2 && gApp.editedFaderValid) {
                    changed = chooseColor(hwnd, gApp.editedFader.indicator);
                } else if (category == 3 && gApp.editedMeterValid) {
                    changed = chooseColor(hwnd, gApp.editedMeter.needle);
                }

                if (changed) {
                    renderPreview();
                }
                return 0;
            }

            if (id == IdPointerColor) {
                const int category = selectedCategory();
                bool changed = false;

                if (category == 0 && gApp.editedKnobValid) {
                    changed = chooseColor(hwnd, gApp.editedKnob.indicator);
                } else if (category == 1 && gApp.editedHardwareValid) {
                    changed = chooseColor(hwnd, gApp.editedHardware.accentOff);
                } else if (category == 2 && gApp.editedFaderValid) {
                    changed = chooseColor(hwnd, gApp.editedFader.tick);
                } else if (category == 3 && gApp.editedMeterValid) {
                    changed = chooseColor(hwnd, gApp.editedMeter.redZone);
                }

                if (changed) {
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
