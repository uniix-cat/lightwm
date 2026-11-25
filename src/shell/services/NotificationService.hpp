#pragma once

#include <QDBusContext>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace lightwm::shell {

class NotificationService : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY notificationsChanged)

public:
    explicit NotificationService(QObject* parent = nullptr);

    QVariantList notifications() const { return m_notifications; }
    Q_INVOKABLE void dismiss(uint id);
    Q_INVOKABLE void clearAll();

public slots:
    uint Notify(const QString& appName,
                uint replacesId,
                const QString& appIcon,
                const QString& summary,
                const QString& body,
                const QStringList& actions,
                const QVariantMap& hints,
                int expireTimeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities();
    QString GetServerInformation(QString& vendor, QString& version, QString& specVersion);

signals:
    void notificationsChanged();
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString& actionKey);

private:
    QVariantList m_notifications;
    uint m_nextId = 1;
};

} // namespace lightwm::shell
