#pragma once

#include <QObject>
#include <QPointer>
#include <QtWaylandClient/private/qwaylandshellintegration_p.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

struct ext_session_lock_manager_v1;
struct ext_session_lock_v1;
struct ext_session_lock_surface_v1;
struct wl_display;
struct wl_registry;

namespace lightwm::shell {

class SessionLockShellIntegration;

class SessionLockSurface : public QtWaylandClient::QWaylandShellSurface {
    Q_OBJECT
public:
    SessionLockSurface(
        QtWaylandClient::QWaylandWindow* window,
        ext_session_lock_v1* lock);
    ~SessionLockSurface() override;

    bool isExposed() const override { return m_configured; }
    bool commitSurfaceRole() const override { return false; }
    bool requestActivate() override { return true; }
    bool requestActivateOnShow() override { return true; }
    bool wantsDecorations() const override { return false; }

    void handleConfigure(uint32_t serial, uint32_t width, uint32_t height);

private:
    ext_session_lock_surface_v1* m_lockSurface = nullptr;
    bool m_configured = false;
};

class SessionLockManager : public QObject {
    Q_OBJECT
public:
    explicit SessionLockManager(QObject* parent = nullptr);
    ~SessionLockManager() override;

    bool isSupported() const { return m_manager != nullptr; }
    bool isLocked() const { return m_locked; }
    bool hasActiveLock() const { return m_lock != nullptr; }

    bool beginLock();
    void unlockAndDestroy();
    void attachToWindow(QWindow* window);

    void onRegistryGlobal(wl_registry* reg, uint32_t name, const char* iface, uint32_t version);
    void onSessionLocked();
    void onSessionFinished();

signals:
    void lockedStateChanged(bool locked);
    void lockFailed();

private:
    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    ext_session_lock_manager_v1* m_manager = nullptr;
    ext_session_lock_v1* m_lock = nullptr;
    bool m_locked = false;

    SessionLockShellIntegration* m_shellIntegration = nullptr;
    friend class SessionLockShellIntegration;
};

class SessionLockShellIntegration : public QtWaylandClient::QWaylandShellIntegration {
public:
    explicit SessionLockShellIntegration(SessionLockManager* mgr) : m_mgr(mgr) {}
    bool initialize(QtWaylandClient::QWaylandDisplay*) override { return true; }
    QtWaylandClient::QWaylandShellSurface* createShellSurface(QtWaylandClient::QWaylandWindow* window) override;

private:
    SessionLockManager* m_mgr = nullptr;
};

} // namespace lightwm::shell
