#pragma once

#include <algorithm>
#include <cmath>

namespace knob125a {

struct AssetGeometryPolicy {
    // Fraction of the cell reserved for visible asset geometry.
    // Remaining space acts as a deterministic safe margin.
    float safeAreaRatio {0.90f};

    // Optical offsets are expressed as fractions of half the cell size.
    // They stay resolution-independent across 1x/1.5x/2x/3x exports.
    float opticalOffsetX {0.0f};
    float opticalOffsetY {0.0f};

    // Rectilinear hardware should land on stable pixel boundaries.
    bool snapHardEdges {true};
};

struct AssetMetrics {
    float size {0.0f};
    float scale {1.0f};
    float cx {0.0f};
    float cy {0.0f};
    float half {0.0f};
    float safeHalf {0.0f};
};

inline float snapPixel(float v) {
    return std::floor(v) + 0.5f;
}

inline AssetMetrics makeAssetMetrics(
    int cellSize,
    int xOffset,
    int yOffset,
    const AssetGeometryPolicy& policy) {

    const float size = static_cast<float>(cellSize);
    const float half = size * 0.5f;
    AssetMetrics m;
    m.size = size;
    m.scale = size / 128.0f;
    m.half = half;
    m.safeHalf = half * std::clamp(policy.safeAreaRatio, 0.50f, 0.98f);
    m.cx = static_cast<float>(xOffset) + half +
           policy.opticalOffsetX * half;
    m.cy = static_cast<float>(yOffset) + half +
           policy.opticalOffsetY * half;
    return m;
}

inline float snapHardEdge(float v, const AssetGeometryPolicy& policy) {
    return policy.snapHardEdges ? snapPixel(v) : v;
}

} // namespace knob125a
