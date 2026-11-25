#include "shell/services/LockscreenService.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QScreen>
#include <QWindow>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <security/pam_appl.h>
#include <unistd.h>

namespace lightwm::shell {

namespace {

int pamConversation(int numMsg, const struct pam_message** msg, struct pam_response** resp, void* appdataPtr) {
    if (numMsg <= 0 || !appdataPtr) return PAM_CONV_ERR;
    const auto* password = static_cast<const char*>(appdataPtr);
    auto* replies = static_cast<struct pam_response*>(std::calloc(static_cast<size_t>(numMsg), sizeof(struct pam_response)));
    if (!replies) return PAM_BUF_ERR;

    for (int i = 0; i < numMsg; ++i) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF || msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            replies[i].resp = ::strdup(password);
        }
    }
    *resp = replies;
    return PAM_SUCCESS;
}

} // namespace

LockscreenService::LockscreenService(QObject* parent) : QObject(parent) {
    connect(&m_sessionLock, &SessionLockManager::lockFailed, this, [this]() {
        clearWindows();
        if (m_locked) {
            m_locked = false;
            emit lockedChanged();
        }
    });
}

LockscreenService::~LockscreenService() {
    clearWindows();
}

QString LockscreenService::username() const {
    if (const passwd* pw = ::getpwuid(::getuid())) {
        return QString::fromLocal8Bit(pw->pw_name);
    }
    return QStringLiteral("user");
}

void LockscreenService::clearWindows() {
    for (const auto& w : m_lockWindows) {
        if (w) {
            w->close();
            w->deleteLater();
        }
    }
    m_lockWindows.clear();
}

void LockscreenService::lock() {
    if (m_locked) return;
    if (!m_engine) return;

    if (!m_component) {
        m_component = new QQmlComponent(
            m_engine,
            QUrl(QStringLiteral("qrc:/qt/qml/LightWM/Shell/qml/lockscreen/LockscreenOverlay.qml")),
            this);
    }

    const bool usingProtocol = m_sessionLock.beginLock();
    m_locked = true;
    emit lockedChanged();

    const auto screens = QGuiApplication::screens();
    for (QScreen* scr : screens) {
        QObject* obj = m_component->create();
        auto* win = qobject_cast<QWindow*>(obj);
        if (!win) {
            delete obj;
            continue;
        }
        win->setScreen(scr);
        win->setGeometry(scr->geometry());
        if (usingProtocol) {
            m_sessionLock.attachToWindow(win);
        }
        win->show();
        m_lockWindows.append(win);
    }
}

bool LockscreenService::verifyPassword(const QString& password) {
    const QByteArray userBytes = username().toLocal8Bit();
    const QByteArray passBytes = password.toUtf8();

    struct pam_conv conv{
        .conv = pamConversation,
        .appdata_ptr = const_cast<char*>(passBytes.constData()),
    };

    pam_handle_t* pamh = nullptr;
    int rc = ::pam_start("system-auth", userBytes.constData(), &conv, &pamh);
    if (rc == PAM_SUCCESS) {
        // Only pam_authenticate is valid for unprivileged lockers (pam_acct_mgmt needs root)
        rc = ::pam_authenticate(pamh, 0);
    }
    const QString errMsg = pamh ? QString::fromLocal8Bit(::pam_strerror(pamh, rc)) : QStringLiteral("PAM error");
    if (pamh) {
        ::pam_end(pamh, rc);
    }

    if (rc == PAM_SUCCESS) {
        clearWindows();
        m_sessionLock.unlockAndDestroy();
        m_locked = false;
        emit lockedChanged();
        return true;
    }

    emit unlockFailed(errMsg);
    return false;
}

} // namespace lightwm::shell
