#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace knob125a {

struct Color {
    std::uint8_t a {255};
    std::uint8_t r {0};
    std::uint8_t g {0};
    std::uint8_t b {0};
};

struct KnobStyle {
    std::wstring name;
    Color shadow;
    Color bezelOuter;
    Color bezelInner;
    Color bodyTop;
    Color bodyBottom;
    Color edge;
    Color highlight;
    Color indicator;
    Color centerCap;
    float shadowOffsetY {4.0f};
    float shadowScale {1.02f};
    float bezelRadius {0.44f};
    float bodyRadius {0.385f};
    float indicatorInnerRadius {0.19f};
    float indicatorOuterRadius {0.32f};
    float indicatorWidth {3.0f};
    bool drawCenterCap {false};
};

struct RenderOptions {
    int cellSize {128};
    int frameCount {128};
    float startAngleDeg {-135.0f};
    float endAngleDeg {135.0f};
};

class KnobRenderer {
public:
    static std::vector<KnobStyle> builtInStyles();

    // Writes one vertical PNG filmstrip.
    // Returns false when the bitmap could not be created or encoded.
    bool renderVerticalFilmstrip(
        const KnobStyle& style,
        const RenderOptions& options,
        const std::wstring& outputPath) const;
};

} // namespace knob125a
