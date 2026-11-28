#pragma once

#include "core/Theme.hpp"
#include <cstdint>
#include <unordered_map>

struct wl_buffer;
struct wl_compositor;
struct wl_region;
struct wl_shm;
struct wl_shm_pool;
struct wl_surface;

namespace lightwm::wm {

// Renders shared anti-aliased rounded corner overlay buffers for river_decoration_v1.
class CornerDecorations {
public:
    CornerDecorations() = default;
    ~CornerDecorations();

    void init(wl_compositor* compositor, wl_shm* shm);
    void rebuild(int radius, int borderWidth, const ThemePalette& palette);
    void destroyBuffers();

    bool isReady() const { return m_compositor && m_shm && m_size > 0; }
    int patchSize() const { return m_size; }
    int pixelSize(uint32_t scale120 = 120) const;
    int borderWidth() const { return m_borderWidth; }
    uint64_t version() const { return m_version; }

    wl_surface* createClickThroughSurface() const;
    wl_buffer* buffer(bool active, int cornerIndex, uint32_t scale120 = 120) const;

private:
    struct ScaleBuffers {
        int pixelSize = 0;
        // [0 = inactive, 1 = active][0 = TL, 1 = TR, 2 = BL, 3 = BR]
        wl_buffer* buffers[2][4]{};
    };

    const ScaleBuffers& ensureScale(uint32_t scale120) const;
    wl_buffer* createCornerBuffer(
        int cornerIndex,
        int radius,
        int borderWidth,
        int pixelSize,
        const QColor& borderCol,
        const QColor& outerMaskCol) const;

    wl_compositor* m_compositor = nullptr;
    wl_shm* m_shm = nullptr;

    int m_radius = 0;
    int m_size = 0;
    int m_borderWidth = 0;
    uint64_t m_version = 0;
    ThemePalette m_palette;

    mutable std::unordered_map<uint32_t, ScaleBuffers> m_scaleCache;
};

} // namespace lightwm::wm
