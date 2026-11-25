#include "shell/services/BluetoothService.hpp"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>

namespace lightwm::shell {

namespace {
constexpr auto kBluezService = "org.bluez";
constexpr auto kPropsIface = "org.freedesktop.DBus.Properties";
} // namespace

BluetoothService::BluetoothService(QObject* parent) : QObject(parent) {
    refresh();
}

QString BluetoothService::findAdapterPath() const {
    return QStringLiteral("/org/bluez/hci0");
}

void BluetoothService::setPowered(bool powered) {
    QDBusInterface props(
        QString::fromLatin1(kBluezService),
        findAdapterPath(),
        QString::fromLatin1(kPropsIface),
        QDBusConnection::systemBus());
    if (props.isValid()) {
        props.call(
            QStringLiteral("Set"),
            QStringLiteral("org.bluez.Adapter1"),
            QStringLiteral("Powered"),
            QVariant::fromValue(QDBusVariant(powered)));
    }
    m_powered = powered;
    emit stateChanged();
    refresh();
}

void BluetoothService::refresh() {
    m_devices.clear();

    QDBusInterface adapterProps(
        QString::fromLatin1(kBluezService),
        findAdapterPath(),
        QString::fromLatin1(kPropsIface),
        QDBusConnection::systemBus());
    if (adapterProps.isValid()) {
        QDBusReply<QDBusVariant> pow = adapterProps.call(
            QStringLiteral("Get"),
            QStringLiteral("org.bluez.Adapter1"),
            QStringLiteral("Powered"));
        if (pow.isValid()) m_powered = pow.value().variant().toBool();

        QDBusReply<QDBusVariant> disc = adapterProps.call(
            QStringLiteral("Get"),
            QStringLiteral("org.bluez.Adapter1"),
            QStringLiteral("Discovering"));
        if (disc.isValid()) m_scanning = disc.value().variant().toBool();
    }

    QDBusInterface om(
        QString::fromLatin1(kBluezService),
        QStringLiteral("/"),
        QStringLiteral("org.freedesktop.DBus.ObjectManager"),
        QDBusConnection::systemBus());
    if (om.isValid()) {
        const QDBusMessage reply = om.call(QStringLiteral("GetManagedObjects"));
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            const auto arg = reply.arguments().at(0).value<QDBusArgument>();
            arg.beginMap();
            while (!arg.atEnd()) {
                QDBusObjectPath path;
                QMap<QString, QVariantMap> ifaces;
                arg.beginMapEntry();
                arg >> path >> ifaces;
                arg.endMapEntry();

                if (ifaces.contains(QStringLiteral("org.bluez.Device1"))) {
                    const QVariantMap dev = ifaces.value(QStringLiteral("org.bluez.Device1"));
                    const QString name = dev.value(QStringLiteral("Alias")).toString();
                    if (name.isEmpty()) continue;

                    QVariantMap entry;
                    entry.insert(QStringLiteral("path"), path.path());
                    entry.insert(QStringLiteral("name"), name);
                    entry.insert(QStringLiteral("connected"), dev.value(QStringLiteral("Connected")).toBool());
                    entry.insert(QStringLiteral("paired"), dev.value(QStringLiteral("Paired")).toBool());
                    m_devices.append(entry);
                }
            }
            arg.endMap();
        }
    }

    emit stateChanged();
    emit devicesChanged();
}

void BluetoothService::toggleDiscovery() {
    QDBusInterface adapter(
        QString::fromLatin1(kBluezService),
        findAdapterPath(),
        QStringLiteral("org.bluez.Adapter1"),
        QDBusConnection::systemBus());
    if (!adapter.isValid()) return;
    adapter.call(m_scanning ? QStringLiteral("StopDiscovery") : QStringLiteral("StartDiscovery"));
    refresh();
}

void BluetoothService::toggleDeviceConnection(const QString& objectPath) {
    QDBusInterface props(
        QString::fromLatin1(kBluezService),
        objectPath,
        QString::fromLatin1(kPropsIface),
        QDBusConnection::systemBus());
    QDBusReply<QDBusVariant> conn = props.call(
        QStringLiteral("Get"),
        QStringLiteral("org.bluez.Device1"),
        QStringLiteral("Connected"));
    const bool isConnected = conn.isValid() && conn.value().variant().toBool();

    QDBusInterface dev(
        QString::fromLatin1(kBluezService),
        objectPath,
        QStringLiteral("org.bluez.Device1"),
        QDBusConnection::systemBus());
    dev.call(isConnected ? QStringLiteral("Disconnect") : QStringLiteral("Connect"));
    refresh();
}

} // namespace lightwm::shell
