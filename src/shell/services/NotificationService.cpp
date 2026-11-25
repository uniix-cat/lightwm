#include "shell/services/NotificationService.hpp"

#include <QDBusConnection>
#include <QTimer>

namespace lightwm::shell {

NotificationService::NotificationService(QObject* parent) : QObject(parent) {
    auto bus = QDBusConnection::sessionBus();
    bus.registerService(QStringLiteral("org.freedesktop.Notifications"));
    bus.registerObject(
        QStringLiteral("/org/freedesktop/Notifications"),
        this,
        QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
}

uint NotificationService::Notify(
    const QString& appName,
    uint replacesId,
    const QString& appIcon,
    const QString& summary,
    const QString& body,
    const QStringList& actions,
    const QVariantMap& hints,
    int expireTimeout) {
    (void)actions;
    (void)hints;

    const uint id = replacesId ? replacesId : m_nextId++;
    if (replacesId) {
        for (int i = 0; i < m_notifications.size(); ++i) {
            if (m_notifications[i].toMap().value(QStringLiteral("id")).toUInt() == replacesId) {
                m_notifications.removeAt(i);
                break;
            }
        }
    }

    QVariantMap n;
    n.insert(QStringLiteral("id"), id);
    n.insert(QStringLiteral("appName"), appName);
    n.insert(QStringLiteral("appIcon"), appIcon);
    n.insert(QStringLiteral("summary"), summary);
    n.insert(QStringLiteral("body"), body);
    m_notifications.prepend(n);
    emit notificationsChanged();

    const int timeoutMs = (expireTimeout > 0) ? expireTimeout : 5000;
    QTimer::singleShot(timeoutMs, this, [this, id]() {
        dismiss(id);
    });
    return id;
}

void NotificationService::CloseNotification(uint id) {
    dismiss(id);
}

QStringList NotificationService::GetCapabilities() {
    return {QStringLiteral("body"), QStringLiteral("body-markup"), QStringLiteral("persistence")};
}

QString NotificationService::GetServerInformation(QString& vendor, QString& version, QString& specVersion) {
    vendor = QStringLiteral("LightWM");
    version = QStringLiteral("0.1.0");
    specVersion = QStringLiteral("1.2");
    return QStringLiteral("lightwm-shell");
}

void NotificationService::dismiss(uint id) {
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].toMap().value(QStringLiteral("id")).toUInt() == id) {
            m_notifications.removeAt(i);
            emit notificationsChanged();
            emit NotificationClosed(id, 2);
            return;
        }
    }
}

void NotificationService::clearAll() {
    m_notifications.clear();
    emit notificationsChanged();
}

} // namespace lightwm::shell
