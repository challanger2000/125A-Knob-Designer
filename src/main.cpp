#include "KnobRenderer.h"
#include "HardwareRenderer.h"
#include "FaderRenderer.h"
#include "MeterRenderer.h"

#include <windows.h>

#include <filesystem>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::wstring safeFilename(std::wstring name) {
    for (auto& ch : name) {
        if (ch == L' ') {
            ch = L'_';
        }
    }
    return name;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    using namespace knob125a;

    RenderOptions options;
    fs::path outputDir = L"exports";
    const bool explicitCellSize = argc >= 3;

    if (argc >= 2) {
        outputDir = argv[1];
    }
    if (explicitCellSize) {
        options.cellSize = std::max(16, _wtoi(argv[2]));
    }
    if (argc >= 4) {
        options.frameCount = std::max(2, _wtoi(argv[3]));
    }

    std::error_code ec;
    fs::create_directories(outputDir, ec);
    if (ec) {
        std::wcerr << L"Could not create output directory: "
                   << outputDir.wstring() << L"\n";
        return 2;
    }

    KnobRenderer renderer;
    const auto styles = KnobRenderer::builtInStyles();

    const std::vector<float> scaleFactors =
        explicitCellSize
            ? std::vector<float>{1.0f}
            : std::vector<float>{1.0f, 1.5f, 2.0f, 3.0f};

    int failures = 0;
    for (const auto& style : styles) {
        for (const float scaleFactor : scaleFactors) {
            RenderOptions styleOptions = options;

            if (!explicitCellSize) {
                styleOptions.cellSize = std::max(
                    16,
                    static_cast<int>(
                        std::lround(
                            static_cast<double>(style.preferredCellSize) *
                            static_cast<double>(scaleFactor))));
            }

            const int scalePercent =
                static_cast<int>(std::lround(scaleFactor * 100.0f));

            const auto file =
                outputDir /
                (L"125A_" + safeFilename(style.name) + L"_" +
                 std::to_wstring(styleOptions.cellSize) + L"px_" +
                 std::to_wstring(scalePercent) + L"pct_" +
                 std::to_wstring(styleOptions.frameCount) + L"f.png");

            std::wcout << L"Rendering " << style.name
                       << L" @ " << scalePercent << L"%"
                       << L" -> " << file.wstring() << L"\n";

            if (!renderer.renderVerticalFilmstrip(
                    style, styleOptions, file.wstring())) {
                ++failures;
                std::wcerr << L"FAILED: " << style.name
                           << L" @ " << scalePercent << L"%\n";
            }
        }
    }

    HardwareRenderer hardwareRenderer;
    const auto hardwareStyles = HardwareRenderer::builtInStyles();

    for (const auto& style : hardwareStyles) {
        for (const float scaleFactor : scaleFactors) {
            const int cellSize = explicitCellSize
                ? options.cellSize
                : std::max(
                    16,
                    static_cast<int>(
                        std::lround(
                            static_cast<double>(style.preferredCellSize) *
                            static_cast<double>(scaleFactor))));

            const int scalePercent =
                static_cast<int>(std::lround(scaleFactor * 100.0f));

            const auto file =
                outputDir /
                (L"125A_" + safeFilename(style.name) + L"_" +
                 std::to_wstring(cellSize) + L"px_" +
                 std::to_wstring(scalePercent) + L"pct_" +
                 std::to_wstring(style.stateCount) + L"states.png");

            std::wcout << L"Rendering " << style.name
                       << L" @ " << scalePercent << L"%"
                       << L" -> " << file.wstring() << L"\n";

            if (!hardwareRenderer.renderVerticalFilmstrip(
                    style, cellSize, file.wstring())) {
                ++failures;
                std::wcerr << L"FAILED: " << style.name
                           << L" @ " << scalePercent << L"%\n";
            }
        }
    }

    FaderRenderer faderRenderer;
    const auto faderStyles = FaderRenderer::builtInStyles();

    for (const auto& style : faderStyles) {
        for (const float scaleFactor : scaleFactors) {
            const int width = explicitCellSize
                ? options.cellSize
                : std::max(
                    16,
                    static_cast<int>(std::lround(
                        static_cast<double>(style.preferredWidth) *
                        static_cast<double>(scaleFactor))));

            const int height = explicitCellSize
                ? std::max(
                    32,
                    static_cast<int>(std::lround(
                        static_cast<double>(options.cellSize) *
                        static_cast<double>(style.preferredHeight) /
                        static_cast<double>(style.preferredWidth))))
                : std::max(
                    32,
                    static_cast<int>(std::lround(
                        static_cast<double>(style.preferredHeight) *
                        static_cast<double>(scaleFactor))));

            const int scalePercent =
                static_cast<int>(std::lround(scaleFactor * 100.0f));

            const auto file =
                outputDir /
                (L"125A_" + safeFilename(style.name) + L"_" +
                 std::to_wstring(width) + L"x" +
                 std::to_wstring(height) + L"px_" +
                 std::to_wstring(scalePercent) + L"pct_" +
                 std::to_wstring(style.frameCount) + L"f.png");

            std::wcout << L"Rendering " << style.name
                       << L" @ " << scalePercent << L"%"
                       << L" -> " << file.wstring() << L"\n";

            if (!faderRenderer.renderVerticalFilmstrip(
                    style, width, height, style.frameCount, file.wstring())) {
                ++failures;
                std::wcerr << L"FAILED: " << style.name
                           << L" @ " << scalePercent << L"%\n";
            }
        }
    }

    MeterRenderer meterRenderer;
    const auto meterStyles = MeterRenderer::builtInStyles();

    for (const auto& style : meterStyles) {
        for (const float scaleFactor : scaleFactors) {
            const int width = explicitCellSize
                ? options.cellSize
                : std::max(
                    32,
                    static_cast<int>(std::lround(
                        static_cast<double>(style.preferredWidth) *
                        static_cast<double>(scaleFactor))));

            const int height = explicitCellSize
                ? std::max(
                    24,
                    static_cast<int>(std::lround(
                        static_cast<double>(options.cellSize) *
                        static_cast<double>(style.preferredHeight) /
                        static_cast<double>(style.preferredWidth))))
                : std::max(
                    24,
                    static_cast<int>(std::lround(
                        static_cast<double>(style.preferredHeight) *
                        static_cast<double>(scaleFactor))));

            const int scalePercent =
                static_cast<int>(std::lround(scaleFactor * 100.0f));

            const auto file =
                outputDir /
                (L"125A_" + safeFilename(style.name) + L"_" +
                 std::to_wstring(width) + L"x" +
                 std::to_wstring(height) + L"px_" +
                 std::to_wstring(scalePercent) + L"pct_" +
                 std::to_wstring(style.frameCount) + L"f.png");

            std::wcout << L"Rendering " << style.name
                       << L" @ " << scalePercent << L"%"
                       << L" -> " << file.wstring() << L"\n";

            if (!meterRenderer.renderVerticalFilmstrip(
                    style, width, height, style.frameCount, file.wstring())) {
                ++failures;
                std::wcerr << L"FAILED: " << style.name
                           << L" @ " << scalePercent << L"%\n";
            }
        }
    }

    if (failures != 0) {
        std::wcerr << failures << L" export(s) failed.\n";
        return 1;
    }

    std::wcout << L"All GUI asset filmstrips exported successfully.\n";
    return 0;
}
