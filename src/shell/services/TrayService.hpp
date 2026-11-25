#pragma once

#include <QDBusContext>
#include <QObject>
#include <QStringList>
#include <QVariantList>

namespace lightwm::shell {

// Minimal StatusNotifierWatcher (SNI) host for system tray icons in the bar.
class TrayService : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
    Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ registeredItems NOTIFY itemsChanged)
    Q_PROPERTY(bool IsStatusNotifierHostRegistered READ isHostRegistered CONSTANT)
    Q_PROPERTY(int ProtocolVersion READ protocolVersion CONSTANT)
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)

public:
    explicit TrayService(QObject* parent = nullptr);

    QStringList registeredItems() const { return m_registeredServices; }
    bool isHostRegistered() const { return true; }
    int protocolVersion() const { return 0; }
    QVariantList items() const { return m_items; }

    Q_INVOKABLE void activateItem(const QString& service, const QString& path);

public slots:
    void RegisterStatusNotifierItem(const QString& serviceOrPath);
    void RegisterStatusNotifierHost(const QString& service);

signals:
    void itemsChanged();
    void StatusNotifierItemRegistered(const QString& service);
    void StatusNotifierItemUnregistered(const QString& service);
    void StatusNotifierHostRegistered();

private:
    void refreshItemMetadata(const QString& service, const QString& path);

    QStringList m_registeredServices;
    QVariantList m_items;
};

} // namespace lightwm::shell
