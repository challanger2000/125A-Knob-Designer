#pragma once

#include "KnobRenderer.h"
#include "AssetGeometry.h"

#include <string>
#include <vector>

namespace knob125a {

enum class HardwareAssetKind {
    Led,
    PushButton,
    ToggleSwitch,
    RockerSwitch
};

struct HardwareAssetStyle {
    std::wstring name;
    HardwareAssetKind kind {HardwareAssetKind::Led};
    Color panel {255, 24, 28, 32};
    Color metalTop {255, 92, 98, 103};
    Color metalBottom {255, 28, 31, 34};
    Color faceTop {255, 58, 61, 64};
    Color faceBottom {255, 15, 17, 19};
    Color accentOff {255, 70, 24, 20};
    Color accentOn {255, 236, 58, 43};
    Color highlight {110, 255, 255, 255};
    Color shadow {120, 0, 0, 0};
    int preferredCellSize {64};
    int stateCount {2};
    AssetGeometryPolicy geometry {};

};

class HardwareRenderer {
public:
    static std::vector<HardwareAssetStyle> builtInStyles();

    // Writes one transparent vertical PNG filmstrip containing all states.
    bool renderVerticalFilmstrip(
        const HardwareAssetStyle& style,
        int cellSize,
        const std::wstring& outputPath) const;
};

} // namespace knob125a
