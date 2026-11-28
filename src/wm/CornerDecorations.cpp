#include "wm/CornerDecorations.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

namespace lightwm::wm {

namespace {

int createShmFile(size_t size) {
    int fd = ::memfd_create("lightwm-corners", MFD_CLOEXEC);
    if (fd < 0) return -1;
    if (::ftruncate(fd, static_cast<off_t>(size)) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

uint32_t premulArgb(double a, double r, double g, double b) {
    auto ch = [](double v) -> uint32_t {
        return static_cast<uint32_t>(std::clamp(std::lround(v * 255.0), 0l, 255l));
    };
    const double ac = std::clamp(a, 0.0, 1.0);
    return (ch(ac) << 24) | (ch(r * ac) << 16) | (ch(g * ac) << 8) | ch(b * ac);
}

} // namespace

CornerDecorations::~CornerDecorations() {
    destroyBuffers();
}

void CornerDecorations::init(wl_compositor* compositor, wl_shm* shm) {
    m_compositor = compositor;
    m_shm = shm;
}

void CornerDecorations::destroyBuffers() {
    for (auto& [scale120, entry] : m_scaleCache) {
        (void)scale120;
        for (int s = 0; s < 2; ++s) {
            for (int c = 0; c < 4; ++c) {
                if (entry.buffers[s][c]) {
                    wl_buffer_destroy(entry.buffers[s][c]);
                    entry.buffers[s][c] = nullptr;
                }
            }
        }
    }
    m_scaleCache.clear();
    m_size = 0;
}

void CornerDecorations::rebuild(int radius, int borderWidth, const ThemePalette& palette) {
    destroyBuffers();
    m_borderWidth = std::max(0, borderWidth);
    m_radius = std::clamp(radius, 0, 24);
    m_palette = palette;
    if (!m_compositor || !m_shm || m_radius <= 0) {
        ++m_version;
        return;
    }

    m_size = m_radius + m_borderWidth;
    (void)ensureScale(120);
    ++m_version;
}

int CornerDecorations::pixelSize(uint32_t scale120) const {
    if (m_size <= 0) return 0;
    const uint32_t clamped = std::clamp(scale120 ? scale120 : 120u, 60u, 480u);
    const double scale = static_cast<double>(clamped) / 120.0;
    return std::max(1, static_cast<int>(std::lround(static_cast<double>(m_size) * scale)));
}

const CornerDecorations::ScaleBuffers& CornerDecorations::ensureScale(uint32_t scale120) const {
    const uint32_t clamped = std::clamp(scale120 ? scale120 : 120u, 60u, 480u);
    auto it = m_scaleCache.find(clamped);
    if (it != m_scaleCache.end()) {
        return it->second;
    }

    ScaleBuffers sb;
    sb.pixelSize = pixelSize(clamped);
    if (m_compositor && m_shm && m_radius > 0 && sb.pixelSize > 0) {
        for (int c = 0; c < 4; ++c) {
            sb.buffers[0][c] = createCornerBuffer(c, m_radius, m_borderWidth, sb.pixelSize, m_palette.borderInactive, m_palette.background);
            sb.buffers[1][c] = createCornerBuffer(c, m_radius, m_borderWidth, sb.pixelSize, m_palette.borderActive, m_palette.background);
        }
    }
    auto [inserted, _] = m_scaleCache.emplace(clamped, sb);
    return inserted->second;
}

wl_buffer* CornerDecorations::createCornerBuffer(
    int cornerIndex,
    int radius,
    int borderWidth,
    int pixelSize,
    const QColor& borderCol,
    const QColor& outerMaskCol) const {
    const int logicalS = radius + borderWidth;
    const int P = pixelSize;
    const size_t stride = static_cast<size_t>(P) * 4;
    const size_t byteSize = stride * static_cast<size_t>(P);

    const int fd = createShmFile(byteSize);
    if (fd < 0) return nullptr;

    void* mem = ::mmap(nullptr, byteSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        ::close(fd);
        return nullptr;
    }

    auto* pixels = static_cast<uint32_t*>(mem);

    // Inner arc center in buffer pixel coordinates
    const double cx = (cornerIndex == 0 || cornerIndex == 2) ? static_cast<double>(P) : 0.0;
    const double cy = (cornerIndex == 0 || cornerIndex == 1) ? static_cast<double>(P) : 0.0;

    const double effScale = (logicalS > 0) ? (static_cast<double>(P) / static_cast<double>(logicalS)) : 1.0;
    const double rIn = static_cast<double>(radius) * effScale;
    const double rOut = static_cast<double>(logicalS) * effScale;

    const double br = borderCol.redF();
    const double bg = borderCol.greenF();
    const double bb = borderCol.blueF();

    const double mr = outerMaskCol.redF();
    const double mg = outerMaskCol.greenF();
    const double mb = outerMaskCol.blueF();

    for (int y = 0; y < P; ++y) {
        for (int x = 0; x < P; ++x) {
            const double dx = (x + 0.5) - cx;
            const double dy = (y + 0.5) - cy;
            const double d = std::hypot(dx, dy);

            uint32_t px = 0x00000000u;
            if (d <= rIn - 0.5) {
                // Inside window content: transparent
                px = 0x00000000u;
            } else if (d < rIn + 0.5) {
                // Anti-aliased inner edge between transparent and border (or mask if bw==0)
                const double t = std::clamp(d - (rIn - 0.5), 0.0, 1.0);
                if (borderWidth > 0) {
                    px = premulArgb(t, br, bg, bb);
                } else {
                    px = premulArgb(t, mr, mg, mb);
                }
            } else if (d <= rOut - 0.5) {
                // Solid rounded border
                px = premulArgb(1.0, br, bg, bb);
            } else if (d < rOut + 0.5) {
                // Anti-aliased outer edge between border and outer mask color
                const double t = std::clamp(d - (rOut - 0.5), 0.0, 1.0);
                const double rMix = br * (1.0 - t) + mr * t;
                const double gMix = bg * (1.0 - t) + mg * t;
                const double bMix = bb * (1.0 - t) + mb * t;
                px = premulArgb(1.0, rMix, gMix, bMix);
            } else {
                // Outer corner cap masking rectangular window corner
                px = premulArgb(1.0, mr, mg, mb);
            }

            pixels[y * P + x] = px;
        }
    }

    ::munmap(mem, byteSize);

    wl_shm_pool* pool = wl_shm_create_pool(m_shm, fd, static_cast<int32_t>(byteSize));
    wl_buffer* buf = wl_shm_pool_create_buffer(
        pool, 0, P, P, static_cast<int32_t>(stride), WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    ::close(fd);
    return buf;
}

wl_surface* CornerDecorations::createClickThroughSurface() const {
    if (!m_compositor) return nullptr;
    wl_surface* surf = wl_compositor_create_surface(m_compositor);
    if (!surf) return nullptr;

    // Empty input region so clicks pass straight through the corners to the window
    wl_region* emptyRegion = wl_compositor_create_region(m_compositor);
    wl_surface_set_input_region(surf, emptyRegion);
    wl_region_destroy(emptyRegion);
    return surf;
}

wl_buffer* CornerDecorations::buffer(bool active, int cornerIndex, uint32_t scale120) const {
    if (cornerIndex < 0 || cornerIndex >= 4 || !isReady()) return nullptr;
    const ScaleBuffers& sb = ensureScale(scale120);
    return sb.buffers[active ? 1 : 0][cornerIndex];
}

} // namespace lightwm::wm
