#include "shell/services/TrayService.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QVariantMap>

namespace lightwm::shell {

TrayService::TrayService(QObject* parent) : QObject(parent) {
    auto bus = QDBusConnection::sessionBus();
    bus.registerService(QStringLiteral("org.kde.StatusNotifierWatcher"));
    bus.registerObject(
        QStringLiteral("/StatusNotifierWatcher"),
        this,
        QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties | QDBusConnection::ExportAllSignals);
}

void TrayService::RegisterStatusNotifierItem(const QString& serviceOrPath) {
    QString service = serviceOrPath;
    QString path = QStringLiteral("/StatusNotifierItem");
    if (serviceOrPath.startsWith('/')) {
        service = calledFromDBus() ? message().service() : QString();
        path = serviceOrPath;
    }
    if (service.isEmpty()) return;

    const QString key = service + path;
    if (!m_registeredServices.contains(key)) {
        m_registeredServices.append(key);
        refreshItemMetadata(service, path);
        emit StatusNotifierItemRegistered(key);
    }
}

void TrayService::RegisterStatusNotifierHost(const QString& service) {
    (void)service;
    emit StatusNotifierHostRegistered();
}

void TrayService::refreshItemMetadata(const QString& service, const QString& path) {
    QDBusInterface props(
        service,
        path,
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QDBusConnection::sessionBus());

    QString title = service;
    QString iconName = QStringLiteral("application-x-executable");
    if (props.isValid()) {
        QDBusReply<QDBusVariant> t = props.call(QStringLiteral("Get"), QStringLiteral("org.kde.StatusNotifierItem"), QStringLiteral("Title"));
        if (t.isValid() && !t.value().variant().toString().isEmpty()) title = t.value().variant().toString();

        QDBusReply<QDBusVariant> ic = props.call(QStringLiteral("Get"), QStringLiteral("org.kde.StatusNotifierItem"), QStringLiteral("IconName"));
        if (ic.isValid() && !ic.value().variant().toString().isEmpty()) iconName = ic.value().variant().toString();
    }

    QVariantMap entry;
    entry.insert(QStringLiteral("service"), service);
    entry.insert(QStringLiteral("path"), path);
    entry.insert(QStringLiteral("title"), title);
    entry.insert(QStringLiteral("iconName"), iconName);
    m_items.append(entry);
    emit itemsChanged();
}

void TrayService::activateItem(const QString& service, const QString& path) {
    QDBusInterface item(
        service,
        path,
        QStringLiteral("org.kde.StatusNotifierItem"),
        QDBusConnection::sessionBus());
    if (item.isValid()) {
        item.asyncCall(QStringLiteral("Activate"), 0, 0);
    }
}

} // namespace lightwm::shell
