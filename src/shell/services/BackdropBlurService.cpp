#include "shell/services/BackdropBlurService.hpp"
#include "wlr-screencopy-unstable-v1-client-protocol.h"

#include <QGuiApplication>
#include <QMutexLocker>
#include <QScreen>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <algorithm>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

namespace lightwm::shell {

struct ScreencopyCapture {
    BackdropBlurService* svc = nullptr;
    QString screenName;
    zwlr_screencopy_frame_v1* frame = nullptr;
    uint32_t format = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    bool hasShmBuffer = false;
    bool copyStarted = false;
    bool yInvert = false;

    int fd = -1;
    void* mem = MAP_FAILED;
    size_t byteSize = 0;
    wl_shm_pool* pool = nullptr;
    wl_buffer* buffer = nullptr;

    void cleanup() {
        if (svc) {
            svc->m_inFlightScreens.remove(screenName);
        }
        if (buffer) {
            wl_buffer_destroy(buffer);
            buffer = nullptr;
        }
        if (pool) {
            wl_shm_pool_destroy(pool);
            pool = nullptr;
        }
        if (mem != MAP_FAILED && byteSize > 0) {
            ::munmap(mem, byteSize);
            mem = MAP_FAILED;
        }
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
        if (frame) {
            zwlr_screencopy_frame_v1_destroy(frame);
            frame = nullptr;
        }
    }

    void beginCopy();
    void finishReady();
};

namespace {

bool isSupportedShmFormat(uint32_t fmt) {
    return fmt == WL_SHM_FORMAT_ARGB8888 || fmt == WL_SHM_FORMAT_XRGB8888 ||
           fmt == WL_SHM_FORMAT_ABGR8888 || fmt == WL_SHM_FORMAT_XBGR8888;
}

void regGlobal(void* data, wl_registry*, uint32_t name, const char* iface, uint32_t version) {
    static_cast<BackdropBlurService*>(data)->onRegistryGlobal(name, iface, version);
}

void regRemove(void*, wl_registry*, uint32_t) {}

const wl_registry_listener kRegListener = {
    .global = regGlobal,
    .global_remove = regRemove,
};

void frameBuffer(void* data, zwlr_screencopy_frame_v1*, uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
    auto* cap = static_cast<ScreencopyCapture*>(data);
    if (!cap->hasShmBuffer && isSupportedShmFormat(format) && width > 0 && height > 0) {
        cap->format = format;
        cap->width = width;
        cap->height = height;
        cap->stride = stride;
        cap->hasShmBuffer = true;
    }
    if (cap->svc && cap->svc->screencopyVersion() < 3 && !cap->copyStarted) {
        cap->beginCopy();
    }
}

void frameFlags(void* data, zwlr_screencopy_frame_v1*, uint32_t flags) {
    static_cast<ScreencopyCapture*>(data)->yInvert = (flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT) != 0;
}

void frameReady(void* data, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {
    auto* cap = static_cast<ScreencopyCapture*>(data);
    cap->finishReady();
    cap->cleanup();
    delete cap;
}

void frameFailed(void* data, zwlr_screencopy_frame_v1*) {
    auto* cap = static_cast<ScreencopyCapture*>(data);
    cap->cleanup();
    delete cap;
}

void frameDamage(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t, uint32_t) {}
void frameLinuxDmabuf(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {}

void frameBufferDone(void* data, zwlr_screencopy_frame_v1*) {
    auto* cap = static_cast<ScreencopyCapture*>(data);
    if (!cap->copyStarted) {
        cap->beginCopy();
    }
}

const zwlr_screencopy_frame_v1_listener kFrameListener = {
    .buffer = frameBuffer,
    .flags = frameFlags,
    .ready = frameReady,
    .failed = frameFailed,
    .damage = frameDamage,
    .linux_dmabuf = frameLinuxDmabuf,
    .buffer_done = frameBufferDone,
};

} // namespace

void ScreencopyCapture::beginCopy() {
    if (!svc || !svc->m_shm || !hasShmBuffer || copyStarted) {
        cleanup();
        delete this;
        return;
    }

    copyStarted = true;
    byteSize = static_cast<size_t>(stride) * static_cast<size_t>(height);
    fd = ::memfd_create("lightwm-backdrop", MFD_CLOEXEC);
    if (fd < 0 || ::ftruncate(fd, static_cast<off_t>(byteSize)) < 0) {
        cleanup();
        delete this;
        return;
    }

    mem = ::mmap(nullptr, byteSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        cleanup();
        delete this;
        return;
    }

    pool = wl_shm_create_pool(svc->m_shm, fd, static_cast<int32_t>(byteSize));
    buffer = wl_shm_pool_create_buffer(
        pool, 0,
        static_cast<int32_t>(width),
        static_cast<int32_t>(height),
        static_cast<int32_t>(stride),
        format);
    if (!buffer) {
        cleanup();
        delete this;
        return;
    }

    zwlr_screencopy_frame_v1_copy(frame, buffer);
    if (svc->m_display) {
        wl_display_flush(svc->m_display);
    }
}

void ScreencopyCapture::finishReady() {
    if (!svc || !svc->m_store || mem == MAP_FAILED || width == 0 || height == 0) {
        return;
    }

    const bool isRgbaOrder = (format == WL_SHM_FORMAT_ABGR8888 || format == WL_SHM_FORMAT_XBGR8888);
    const QImage::Format qfmt = isRgbaOrder ? QImage::Format_RGBX8888 : QImage::Format_RGB32;

    QImage img(static_cast<const uchar*>(mem),
               static_cast<int>(width),
               static_cast<int>(height),
               static_cast<qsizetype>(stride),
               qfmt);

    QImage copy = yInvert ? img.flipped(Qt::Vertical) : img.copy();
    if (copy.width() > 1280) {
        copy = copy.scaled(copy.width() / 2, copy.height() / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    {
        QMutexLocker lock(&svc->m_store->mutex);
        svc->m_store->frames.insert(screenName, copy);
    }
    ++svc->m_revision;
    emit svc->backdropUpdated();
}

BackdropImageProvider::BackdropImageProvider(std::shared_ptr<BackdropFrameStore> store)
    : QQuickImageProvider(QQuickImageProvider::Image), m_store(std::move(store)) {}

QImage BackdropImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QString key = id;
    const int qIdx = key.indexOf(QLatin1Char('?'));
    if (qIdx >= 0) {
        key = key.left(qIdx);
    }

    QImage img;
    if (m_store) {
        QMutexLocker lock(&m_store->mutex);
        img = m_store->frames.value(key);
    }
    if (img.isNull()) {
        img = QImage(2, 2, QImage::Format_RGB32);
        img.fill(Qt::black);
    }
    if (requestedSize.isValid() && !requestedSize.isEmpty()) {
        img = img.scaled(requestedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    if (size) {
        *size = img.size();
    }
    return img;
}

BackdropBlurService::BackdropBlurService(QObject* parent)
    : QObject(parent), m_store(std::make_shared<BackdropFrameStore>()) {
    if (auto* wlApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
        m_display = wlApp->display();
        if (m_display) {
            m_registry = wl_display_get_registry(m_display);
            wl_registry_add_listener(m_registry, &kRegListener, this);
            wl_display_roundtrip(m_display);
        }
    }
}

BackdropBlurService::~BackdropBlurService() {
    if (m_screencopy) zwlr_screencopy_manager_v1_destroy(m_screencopy);
    if (m_shm) wl_shm_destroy(m_shm);
    if (m_registry) wl_registry_destroy(m_registry);
}

QQuickImageProvider* BackdropBlurService::createImageProvider() const {
    return new BackdropImageProvider(m_store);
}

void BackdropBlurService::onRegistryGlobal(uint32_t name, const char* iface, uint32_t version) {
    if (std::strcmp(iface, zwlr_screencopy_manager_v1_interface.name) == 0) {
        m_screencopyVersion = std::min(version, 3u);
        m_screencopy = static_cast<zwlr_screencopy_manager_v1*>(
            wl_registry_bind(m_registry, name, &zwlr_screencopy_manager_v1_interface, m_screencopyVersion));
    } else if (std::strcmp(iface, wl_shm_interface.name) == 0) {
        m_shm = static_cast<wl_shm*>(wl_registry_bind(m_registry, name, &wl_shm_interface, 1u));
    }
}

bool BackdropBlurService::hasFrame(const QString& screenName) const {
    if (!m_store) return false;
    QMutexLocker lock(&m_store->mutex);
    return m_store->frames.contains(screenName);
}

void BackdropBlurService::capturePrimary() {
    captureScreen(QGuiApplication::primaryScreen());
}

void BackdropBlurService::captureScreen(QScreen* screen) {
    if (!screen || !available()) return;
    const QString name = screen->name();
    if (name.isEmpty() || m_inFlightScreens.contains(name)) return;

    auto* wlScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle());
    if (!wlScreen || !wlScreen->output()) return;

    auto* cap = new ScreencopyCapture();
    cap->svc = this;
    cap->screenName = name;
    cap->frame = zwlr_screencopy_manager_v1_capture_output(m_screencopy, 0, wlScreen->output());
    if (!cap->frame) {
        delete cap;
        return;
    }

    m_inFlightScreens.insert(name);
    zwlr_screencopy_frame_v1_add_listener(cap->frame, &kFrameListener, cap);
    if (m_display) {
        wl_display_flush(m_display);
    }
}

} // namespace lightwm::shell
