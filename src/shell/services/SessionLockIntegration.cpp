#include "shell/services/SessionLockIntegration.hpp"
#include "ext-session-lock-v1-client-protocol.h"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <algorithm>
#include <cstring>
#include <wayland-client.h>

namespace lightwm::shell {

namespace {

void lockSurfaceConfigure(void* data, ext_session_lock_surface_v1*, uint32_t serial, uint32_t w, uint32_t h) {
    static_cast<SessionLockSurface*>(data)->handleConfigure(serial, w, h);
}

const ext_session_lock_surface_v1_listener kLockSurfaceListener = {
    .configure = lockSurfaceConfigure,
};

void lockLocked(void* data, ext_session_lock_v1*) {
    static_cast<SessionLockManager*>(data)->onSessionLocked();
}

void lockFinished(void* data, ext_session_lock_v1*) {
    static_cast<SessionLockManager*>(data)->onSessionFinished();
}

const ext_session_lock_v1_listener kLockListener = {
    .locked = lockLocked,
    .finished = lockFinished,
};

void regGlobal(void* data, wl_registry* reg, uint32_t name, const char* iface, uint32_t version) {
    static_cast<SessionLockManager*>(data)->onRegistryGlobal(reg, name, iface, version);
}

void regRemove(void*, wl_registry*, uint32_t) {}

const wl_registry_listener kRegListener = {
    .global = regGlobal,
    .global_remove = regRemove,
};

} // namespace

SessionLockSurface::SessionLockSurface(QtWaylandClient::QWaylandWindow* window, ext_session_lock_v1* lock)
    : QtWaylandClient::QWaylandShellSurface(window) {
    wl_surface* wlSurf = QtWaylandClient::QWaylandShellIntegration::wlSurfaceForWindow(window);
    wl_output* wlOut = nullptr;
    if (auto* scr = window->waylandScreen()) {
        wlOut = scr->output();
    }
    if (lock && wlSurf && wlOut) {
        m_lockSurface = ext_session_lock_v1_get_lock_surface(lock, wlSurf, wlOut);
        ext_session_lock_surface_v1_add_listener(m_lockSurface, &kLockSurfaceListener, this);
    }
}

SessionLockSurface::~SessionLockSurface() {
    if (m_lockSurface) {
        ext_session_lock_surface_v1_destroy(m_lockSurface);
        m_lockSurface = nullptr;
    }
}

void SessionLockSurface::handleConfigure(uint32_t serial, uint32_t width, uint32_t height) {
    if (m_lockSurface) {
        ext_session_lock_surface_v1_ack_configure(m_lockSurface, serial);
    }
    m_configured = true;
    resizeFromApplyConfigure(QSize(static_cast<int>(width), static_cast<int>(height)));
    if (window()) {
        window()->updateExposure();
    }
}

QtWaylandClient::QWaylandShellSurface* SessionLockShellIntegration::createShellSurface(
    QtWaylandClient::QWaylandWindow* window) {
    return new SessionLockSurface(window, m_mgr ? m_mgr->m_lock : nullptr);
}

SessionLockManager::SessionLockManager(QObject* parent) : QObject(parent) {
    m_shellIntegration = new SessionLockShellIntegration(this);

    if (auto* wlApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
        m_display = wlApp->display();
        if (m_display) {
            m_registry = wl_display_get_registry(m_display);
            wl_registry_add_listener(m_registry, &kRegListener, this);
            wl_display_roundtrip(m_display);
        }
    }
}

SessionLockManager::~SessionLockManager() {
    unlockAndDestroy();
    if (m_manager) ext_session_lock_manager_v1_destroy(m_manager);
    if (m_registry) wl_registry_destroy(m_registry);
    delete m_shellIntegration;
}

void SessionLockManager::onRegistryGlobal(wl_registry* reg, uint32_t name, const char* iface, uint32_t version) {
    if (std::strcmp(iface, ext_session_lock_manager_v1_interface.name) == 0) {
        m_manager = static_cast<ext_session_lock_manager_v1*>(
            wl_registry_bind(reg, name, &ext_session_lock_manager_v1_interface, std::min(version, 1u)));
    }
}

bool SessionLockManager::beginLock() {
    if (!m_manager) return false;
    if (m_lock) return true;

    m_locked = false;
    m_lock = ext_session_lock_manager_v1_lock(m_manager);
    ext_session_lock_v1_add_listener(m_lock, &kLockListener, this);
    return true;
}

void SessionLockManager::attachToWindow(QWindow* window) {
    if (!window) return;
    window->create();
    if (auto* wlWin = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle())) {
        wlWin->setShellIntegration(m_shellIntegration);
    }
}

void SessionLockManager::onSessionLocked() {
    m_locked = true;
    emit lockedStateChanged(true);
}

void SessionLockManager::onSessionFinished() {
    if (m_lock) {
        if (m_locked) {
            ext_session_lock_v1_unlock_and_destroy(m_lock);
        } else {
            ext_session_lock_v1_destroy(m_lock);
        }
        m_lock = nullptr;
    }
    m_locked = false;
    emit lockFailed();
    emit lockedStateChanged(false);
}

void SessionLockManager::unlockAndDestroy() {
    if (!m_lock) {
        m_locked = false;
        return;
    }
    if (m_locked) {
        ext_session_lock_v1_unlock_and_destroy(m_lock);
    } else {
        ext_session_lock_v1_destroy(m_lock);
    }
    m_lock = nullptr;
    m_locked = false;
    if (m_display) {
        wl_display_roundtrip(m_display);
    }
    emit lockedStateChanged(false);
}

} // namespace lightwm::shell
