#pragma once

#include "AssetGeometry.h"
#include "KnobRenderer.h"

#include <string>
#include <vector>

namespace knob125a {

struct MeterStyle {
    std::wstring name;
    Color frameTop {255, 62, 68, 72};
    Color frameBottom {255, 15, 18, 21};
    Color faceTop {255, 239, 226, 194};
    Color faceBottom {255, 205, 188, 151};
    Color scale {255, 47, 41, 34};
    Color redZone {255, 184, 44, 34};
    Color needle {255, 42, 35, 29};
    Color glass {42, 255, 255, 255};
    Color shadow {125, 0, 0, 0};
    int preferredWidth {160};
    int preferredHeight {96};
    int frameCount {128};
    float startAngleDeg {-52.0f};
    float endAngleDeg {52.0f};
    AssetGeometryPolicy geometry {};
};

class MeterRenderer {
public:
    static std::vector<MeterStyle> builtInStyles();

    bool renderVerticalFilmstrip(
        const MeterStyle& style,
        int cellWidth,
        int cellHeight,
        int frameCount,
        const std::wstring& outputPath) const;
};

} // namespace knob125a
