#include "wm/LayoutEngine.hpp"

#include <algorithm>
#include <cmath>

namespace lightwm::wm {

namespace {

void splitDwindleRecursive(
    const std::vector<uint64_t>& ids,
    size_t index,
    const Rect& area,
    int gapsInner,
    double splitRatio,
    std::vector<LayoutItem>& out) {
    if (index >= ids.size()) {
        return;
    }

    if (index == ids.size() - 1) {
        out.push_back(LayoutItem{ids[index], area});
        return;
    }

    const int halfGap = gapsInner / 2;
    const int remGap = gapsInner - halfGap;

    if (area.width >= area.height) {
        int leftW = static_cast<int>(std::round(area.width * splitRatio)) - halfGap;
        leftW = std::max(64, std::min(leftW, area.width - gapsInner - 64));
        const int rightX = area.x + leftW + gapsInner;
        const int rightW = std::max(64, area.width - leftW - gapsInner);

        out.push_back(LayoutItem{ids[index], Rect{area.x, area.y, leftW, area.height}});
        splitDwindleRecursive(
            ids,
            index + 1,
            Rect{rightX, area.y, rightW, area.height},
            gapsInner,
            splitRatio,
            out);
        (void)remGap;
    } else {
        int topH = static_cast<int>(std::round(area.height * splitRatio)) - halfGap;
        topH = std::max(64, std::min(topH, area.height - gapsInner - 64));
        const int bottomY = area.y + topH + gapsInner;
        const int bottomH = std::max(64, area.height - topH - gapsInner);

        out.push_back(LayoutItem{ids[index], Rect{area.x, area.y, area.width, topH}});
        splitDwindleRecursive(
            ids,
            index + 1,
            Rect{area.x, bottomY, area.width, bottomH},
            gapsInner,
            splitRatio,
            out);
    }
}

} // namespace

std::vector<LayoutItem> LayoutEngine::computeDwindle(
    const std::vector<uint64_t>& windowIds,
    const Rect& usableArea,
    int gapsInner,
    int gapsOuter,
    double splitRatio) {
    std::vector<LayoutItem> result;
    if (windowIds.empty() || usableArea.width <= 0 || usableArea.height <= 0) {
        return result;
    }

    Rect inner{
        usableArea.x + gapsOuter,
        usableArea.y + gapsOuter,
        std::max(100, usableArea.width - 2 * gapsOuter),
        std::max(100, usableArea.height - 2 * gapsOuter),
    };

    result.reserve(windowIds.size());
    splitDwindleRecursive(windowIds, 0, inner, gapsInner, splitRatio, result);
    return result;
}

} // namespace lightwm::wm
