#pragma once

#include "shell/services/SessionLockIntegration.hpp"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QQmlApplicationEngine;
class QQmlComponent;
class QWindow;

namespace lightwm::shell {

// Manages native ext-session-lock-v1 surfaces per screen and PAM password verification.
class LockscreenService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)
    Q_PROPERTY(QString username READ username CONSTANT)

public:
    explicit LockscreenService(QObject* parent = nullptr);
    ~LockscreenService() override;

    void setEngine(QQmlApplicationEngine* engine) { m_engine = engine; }

    bool locked() const { return m_locked; }
    QString username() const;

    Q_INVOKABLE void lock();
    Q_INVOKABLE bool verifyPassword(const QString& password);

signals:
    void lockedChanged();
    void unlockFailed(const QString& reason);

private:
    void clearWindows();

    SessionLockManager m_sessionLock;
    QQmlApplicationEngine* m_engine = nullptr;
    QQmlComponent* m_component = nullptr;
    QList<QPointer<QWindow>> m_lockWindows;
    bool m_locked = false;
};

} // namespace lightwm::shell
