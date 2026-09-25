#pragma once

#include "AssetGeometry.h"
#include "KnobRenderer.h"

#include <string>
#include <vector>

namespace knob125a {

struct FaderStyle {
    std::wstring name;
    Color shadow {120, 0, 0, 0};
    Color railOuter {255, 18, 20, 23};
    Color railInner {255, 68, 73, 78};
    Color capTop {255, 86, 91, 96};
    Color capBottom {255, 20, 22, 25};
    Color capEdge {255, 5, 6, 8};
    Color indicator {255, 224, 54, 43};
    Color tick {210, 220, 214, 198};
    int preferredWidth {56};
    int preferredHeight {160};
    int frameCount {128};
    AssetGeometryPolicy geometry {};
};

class FaderRenderer {
public:
    static std::vector<FaderStyle> builtInStyles();

    bool renderVerticalFilmstrip(
        const FaderStyle& style,
        int cellWidth,
        int cellHeight,
        int frameCount,
        const std::wstring& outputPath) const;
};

} // namespace knob125a
