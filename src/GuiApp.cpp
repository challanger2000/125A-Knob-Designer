#include "KnobRenderer.h"
#include "HardwareRenderer.h"
#include "FaderRenderer.h"
#include "MeterRenderer.h"

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

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
    IdExport = 1006
};

struct AppState {
    HWND window {};
    HWND category {};
    HWND preset {};
    HWND scale {};
    HWND frame {};
    HWND status {};

    std::unique_ptr<Gdiplus::Image> previewImage;
    fs::path previewPath;

    int frameCount {1};
    int cellWidth {128};
    int cellHeight {128};

    std::vector<KnobStyle> knobStyles;
    std::vector<HardwareAssetStyle> hardwareStyles;
    std::vector<FaderStyle> faderStyles;
    std::vector<MeterStyle> meterStyles;
};

AppState gApp;

std::wstring safeFilename(std::wstring name) {
    for (auto& ch : name) {
        if (ch == L' ') {
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
    return static_cast<int>(
        SendMessageW(gApp.category, CB_GETCURSEL, 0, 0));
}

int selectedPreset() {
    const int index = static_cast<int>(
        SendMessageW(gApp.preset, CB_GETCURSEL, 0, 0));
    return std::max(0, index);
}

void setStatus(const std::wstring& text) {
    SetWindowTextW(gApp.status, text.c_str());
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
}

bool renderSelectedTo(
    const fs::path& outputPath,
    float scaleFactor) {

    const int category = selectedCategory();
    const int preset = selectedPreset();

    if (category == 0) {
        if (preset >= static_cast<int>(gApp.knobStyles.size())) {
            return false;
        }

        const auto& style = gApp.knobStyles[preset];
        RenderOptions options;
        options.cellSize = std::max(
            16,
            static_cast<int>(std::lround(
                static_cast<double>(style.preferredCellSize) *
                static_cast<double>(scaleFactor))));
        options.frameCount = 128;

        gApp.cellWidth = options.cellSize;
        gApp.cellHeight = options.cellSize;
        gApp.frameCount = options.frameCount;

        KnobRenderer renderer;
        return renderer.renderVerticalFilmstrip(
            style, options, outputPath.wstring());
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

    if (category == 0 &&
        preset < static_cast<int>(gApp.knobStyles.size())) {
        return gApp.knobStyles[preset].name;
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
    wchar_t tempPath[MAX_PATH] {};
    const DWORD len = GetTempPathW(MAX_PATH, tempPath);
    if (len == 0 || len >= MAX_PATH) {
        setStatus(L"Preview path could not be created.");
        return false;
    }

    gApp.previewPath =
        fs::path(tempPath) / L"125A_GUI_Asset_Designer_preview.png";

    if (!renderSelectedTo(gApp.previewPath, selectedScale())) {
        setStatus(L"Preview render failed.");
        return false;
    }

    gApp.previewImage.reset(
        Gdiplus::Image::FromFile(gApp.previewPath.c_str(), FALSE));

    if (!gApp.previewImage ||
        gApp.previewImage->GetLastStatus() != Gdiplus::Ok) {
        gApp.previewImage.reset();
        setStatus(L"Preview image could not be loaded.");
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
        L"Preview: " + selectedStyleName() +
        L" | " + std::to_wstring(gApp.cellWidth) +
        L"x" + std::to_wstring(gApp.cellHeight) +
        L" px | " + std::to_wstring(gApp.frameCount) +
        L" Frames/States");

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

        fs::path temp =
            outputDir /
            (baseName + L"_" +
             std::to_wstring(percent) +
             L"pct.png");

        if (!renderSelectedTo(temp, factor)) {
            ++failures;
        }
    }

    if (failures == 0) {
        setStatus(
            L"Export fertig: " + outputDir.wstring());

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

    const int left = 330;
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

    const float sx =
        static_cast<float>(areaW) / srcW;
    const float sy =
        static_cast<float>(areaH) / srcH;
    const float fit = std::min(sx, sy);

    const int dstW =
        std::max(1, static_cast<int>(std::lround(srcW * fit)));
    const int dstH =
        std::max(1, static_cast<int>(std::lround(srcH * fit)));
    const int dstX =
        left + (right - left - dstW) / 2;
    const int dstY =
        top + (bottom - top - dstH) / 2;

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

void createControls(HWND window) {
    HFONT font = static_cast<HFONT>(
        GetStockObject(DEFAULT_GUI_FONT));

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

    makeStatic(L"Preset", 24, 92, 260, 20);
    gApp.preset = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        24, 114, 270, 260,
        window,
        reinterpret_cast<HMENU>(IdPreset),
        nullptr, nullptr);

    makeStatic(L"Auflösung", 24, 156, 260, 20);
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

    HWND renderButton = CreateWindowExW(
        0, L"BUTTON", L"Vorschau rendern",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        24, 314, 270, 36,
        window,
        reinterpret_cast<HMENU>(IdRender),
        nullptr, nullptr);

    HWND exportButton = CreateWindowExW(
        0, L"BUTTON", L"4 Auflösungen exportieren",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        24, 360, 270, 36,
        window,
        reinterpret_cast<HMENU>(IdExport),
        nullptr, nullptr);

    makeStatic(
        L"Renderer: dieselbe Engine wie CI/GitHub\n"
        L"1x / 1.5x / 2x / 3x\n"
        L"Feste Geometrie / Safe Area / Pixel-Snap",
        24, 430, 270, 90);

    gApp.status = CreateWindowExW(
        0, L"STATIC", L"Bereit.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        24, 550, 270, 82,
        window, nullptr, nullptr, nullptr);

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

            if (id == IdCategory &&
                code == CBN_SELCHANGE) {
                populatePresets();
                renderPreview();
                return 0;
            }

            if ((id == IdPreset || id == IdScale) &&
                code == CBN_SELCHANGE) {
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

    HWND window = CreateWindowExW(
        0,
        kWindowClass,
        L"125A GUI Asset Designer v0.1.0",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1120, 720,
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
