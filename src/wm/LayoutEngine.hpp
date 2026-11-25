#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lightwm::wm {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct LayoutItem {
    uint64_t id = 0;
    Rect rect;
};

// Hyprland-style Dwindle (recursive Binary Space Partitioning) layout engine.
// Splits the remaining area along its longer axis on each window, respecting
// inner and outer gaps.
class LayoutEngine {
public:
    static std::vector<LayoutItem> computeDwindle(
        const std::vector<uint64_t>& windowIds,
        const Rect& usableArea,
        int gapsInner,
        int gapsOuter,
        double splitRatio);
};

} // namespace lightwm::wm
