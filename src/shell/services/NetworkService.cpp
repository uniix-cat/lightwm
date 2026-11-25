#include "shell/services/NetworkService.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QTimer>
#include <algorithm>

namespace lightwm::shell {

namespace {
constexpr auto kNmService = "org.freedesktop.NetworkManager";
constexpr auto kNmPath = "/org/freedesktop/NetworkManager";
constexpr auto kNmIface = "org.freedesktop.NetworkManager";
constexpr auto kNmSettingsPath = "/org/freedesktop/NetworkManager/Settings";
constexpr auto kNmSettingsIface = "org.freedesktop.NetworkManager.Settings";
constexpr auto kPropsIface = "org.freedesktop.DBus.Properties";
} // namespace

NetworkService::NetworkService(QObject* parent) : QObject(parent) {
    qDBusRegisterMetaType<NmSettingsMap>();

    auto bus = QDBusConnection::systemBus();
    bus.connect(
        QString::fromLatin1(kNmService),
        QString::fromLatin1(kNmPath),
        QString::fromLatin1(kNmIface),
        QStringLiteral("StateChanged"),
        this,
        SLOT(refresh()));

    refresh();
}

void NetworkService::setStatusMessage(const QString& msg, bool ok) {
    m_statusMessage = msg;
    emit statusChanged();
    emit connectionStatus(msg, ok);
}

QString NetworkService::findWifiDevicePath() const {
    QDBusInterface nm(
        QString::fromLatin1(kNmService),
        QString::fromLatin1(kNmPath),
        QString::fromLatin1(kNmIface),
        QDBusConnection::systemBus());
    if (!nm.isValid()) return {};

    QDBusReply<QList<QDBusObjectPath>> reply = nm.call(QStringLiteral("GetDevices"));
    if (!reply.isValid()) return {};

    for (const QDBusObjectPath& devPath : reply.value()) {
        QDBusInterface props(
            QString::fromLatin1(kNmService),
            devPath.path(),
            QString::fromLatin1(kPropsIface),
            QDBusConnection::systemBus());
        QDBusReply<QDBusVariant> devType = props.call(
            QStringLiteral("Get"),
            QStringLiteral("org.freedesktop.NetworkManager.Device"),
            QStringLiteral("DeviceType"));
        if (devType.isValid() && devType.value().variant().toUInt() == 2) { // NM_DEVICE_TYPE_WIFI
            return devPath.path();
        }
    }
    return {};
}

QString NetworkService::findSavedConnectionForSsid(const QString& ssid) const {
    QDBusInterface settingsMgr(
        QString::fromLatin1(kNmService),
        QString::fromLatin1(kNmSettingsPath),
        QString::fromLatin1(kNmSettingsIface),
        QDBusConnection::systemBus());
    if (!settingsMgr.isValid()) return {};

    QDBusReply<QList<QDBusObjectPath>> list = settingsMgr.call(QStringLiteral("ListConnections"));
    if (!list.isValid()) return {};

    for (const QDBusObjectPath& connPath : list.value()) {
        QDBusInterface conn(
            QString::fromLatin1(kNmService),
            connPath.path(),
            QStringLiteral("org.freedesktop.NetworkManager.Settings.Connection"),
            QDBusConnection::systemBus());
        QDBusReply<NmSettingsMap> reply = conn.call(QStringLiteral("GetSettings"));
        if (!reply.isValid()) continue;

        const NmSettingsMap map = reply.value();
        if (!map.contains(QStringLiteral("802-11-wireless"))) continue;
        const QString savedSsid = QString::fromUtf8(
            map.value(QStringLiteral("802-11-wireless")).value(QStringLiteral("ssid")).toByteArray());
        if (savedSsid == ssid) {
            return connPath.path();
        }
    }
    return {};
}

void NetworkService::setWifiEnabled(bool enabled) {
    QDBusInterface props(
        QString::fromLatin1(kNmService),
        QString::fromLatin1(kNmPath),
        QString::fromLatin1(kPropsIface),
        QDBusConnection::systemBus());
    if (props.isValid()) {
        props.call(
            QStringLiteral("Set"),
            QString::fromLatin1(kNmIface),
            QStringLiteral("WirelessEnabled"),
            QVariant::fromValue(QDBusVariant(enabled)));
    }
    m_wifiEnabled = enabled;
    emit stateChanged();
    QTimer::singleShot(300, this, &NetworkService::refresh);
}

void NetworkService::refresh() {
    m_networks.clear();
    m_activeSsid.clear();

    QDBusInterface nmProps(
        QString::fromLatin1(kNmService),
        QString::fromLatin1(kNmPath),
        QString::fromLatin1(kPropsIface),
        QDBusConnection::systemBus());
    if (nmProps.isValid()) {
        QDBusReply<QDBusVariant> enabled = nmProps.call(
            QStringLiteral("Get"),
            QString::fromLatin1(kNmIface),
            QStringLiteral("WirelessEnabled"));
        if (enabled.isValid()) {
            m_wifiEnabled = enabled.value().variant().toBool();
        }
    }

    const QString devPath = findWifiDevicePath();
    if (devPath.isEmpty()) {
        emit stateChanged();
        emit networksChanged();
        return;
    }

    QDBusInterface wifiProps(
        QString::fromLatin1(kNmService),
        devPath,
        QString::fromLatin1(kPropsIface),
        QDBusConnection::systemBus());
    QString activeApPath;
    if (wifiProps.isValid()) {
        QDBusReply<QDBusVariant> activeAp = wifiProps.call(
            QStringLiteral("Get"),
            QStringLiteral("org.freedesktop.NetworkManager.Device.Wireless"),
            QStringLiteral("ActiveAccessPoint"));
        if (activeAp.isValid()) {
            activeApPath = activeAp.value().variant().value<QDBusObjectPath>().path();
        }
    }

    QDBusInterface wifi(
        QString::fromLatin1(kNmService),
        devPath,
        QStringLiteral("org.freedesktop.NetworkManager.Device.Wireless"),
        QDBusConnection::systemBus());
    QDBusReply<QList<QDBusObjectPath>> aps = wifi.call(QStringLiteral("GetAllAccessPoints"));
    if (aps.isValid()) {
        QMap<QString, QVariantMap> deduped;
        for (const QDBusObjectPath& apPath : aps.value()) {
            QDBusInterface apProps(
                QString::fromLatin1(kNmService),
                apPath.path(),
                QString::fromLatin1(kPropsIface),
                QDBusConnection::systemBus());
            QDBusReply<QVariantMap> all = apProps.call(
                QStringLiteral("GetAll"),
                QStringLiteral("org.freedesktop.NetworkManager.AccessPoint"));
            if (!all.isValid()) continue;

            const QVariantMap propsMap = all.value();
            const QString ssid = QString::fromUtf8(propsMap.value(QStringLiteral("Ssid")).toByteArray());
            if (ssid.isEmpty()) continue;

            const int strength = propsMap.value(QStringLiteral("Strength")).toInt();
            const uint flags = propsMap.value(QStringLiteral("Flags")).toUInt();
            const uint wpaFlags = propsMap.value(QStringLiteral("WpaFlags")).toUInt();
            const uint rsnFlags = propsMap.value(QStringLiteral("RsnFlags")).toUInt();
            const bool secured = (flags != 0 || wpaFlags != 0 || rsnFlags != 0);
            const bool active = (apPath.path() == activeApPath);

            if (active) {
                m_activeSsid = ssid;
            }

            if (!deduped.contains(ssid) || deduped[ssid].value(QStringLiteral("strength")).toInt() < strength || active) {
                QVariantMap item;
                item.insert(QStringLiteral("ssid"), ssid);
                item.insert(QStringLiteral("strength"), strength);
                item.insert(QStringLiteral("secured"), secured);
                item.insert(QStringLiteral("active"), active);
                item.insert(QStringLiteral("saved"), !findSavedConnectionForSsid(ssid).isEmpty());
                item.insert(QStringLiteral("apPath"), apPath.path());
                deduped.insert(ssid, item);
            }
        }

        for (const auto& item : deduped) {
            m_networks.append(item);
        }
        std::sort(m_networks.begin(), m_networks.end(), [](const QVariant& a, const QVariant& b) {
            const auto ma = a.toMap();
            const auto mb = b.toMap();
            if (ma.value(QStringLiteral("active")).toBool() != mb.value(QStringLiteral("active")).toBool()) {
                return ma.value(QStringLiteral("active")).toBool();
            }
            return ma.value(QStringLiteral("strength")).toInt() > mb.value(QStringLiteral("strength")).toInt();
        });
    }

    emit stateChanged();
    emit networksChanged();
}

void NetworkService::rescan() {
    const QString devPath = findWifiDevicePath();
    if (devPath.isEmpty()) return;
    QDBusInterface wifi(
        QString::fromLatin1(kNmService),
        devPath,
        QStringLiteral("org.freedesktop.NetworkManager.Device.Wireless"),
        QDBusConnection::systemBus());
    wifi.call(QStringLiteral("RequestScan"), QVariantMap{});
    setStatusMessage(QStringLiteral("Scanning Wi-Fi..."), true);
    QTimer::singleShot(1200, this, &NetworkService::refresh);
}

void NetworkService::connectToNetwork(const QString& ssid, const QString& password) {
    const QString devPath = findWifiDevicePath();
    if (devPath.isEmpty() || ssid.isEmpty()) {
        setStatusMessage(QStringLiteral("Wi-Fi adapter not found"), false);
        return;
    }

    QString apPath = QStringLiteral("/");
    for (const QVariant& v : m_networks) {
        const auto m = v.toMap();
        if (m.value(QStringLiteral("ssid")).toString() == ssid) {
            apPath = m.value(QStringLiteral("apPath")).toString();
            break;
        }
    }

    QDBusInterface nm(
        QString::fromLatin1(kNmService),
        QString::fromLatin1(kNmPath),
        QString::fromLatin1(kNmIface),
        QDBusConnection::systemBus());

    // Reuse existing saved NetworkManager connection if present and no new password was provided
    if (password.isEmpty()) {
        const QString savedPath = findSavedConnectionForSsid(ssid);
        if (!savedPath.isEmpty()) {
            QDBusReply<QDBusObjectPath> actReply = nm.call(
                QStringLiteral("ActivateConnection"),
                QVariant::fromValue(QDBusObjectPath(savedPath)),
                QVariant::fromValue(QDBusObjectPath(devPath)),
                QVariant::fromValue(QDBusObjectPath(apPath)));
            if (actReply.isValid()) {
                setStatusMessage(QStringLiteral("Connected to ") + ssid, true);
            } else {
                setStatusMessage(actReply.error().message(), false);
            }
            QTimer::singleShot(600, this, &NetworkService::refresh);
            return;
        }
    }

    QVariantMap connSection;
    connSection.insert(QStringLiteral("type"), QStringLiteral("802-11-wireless"));
    connSection.insert(QStringLiteral("id"), ssid);

    QVariantMap wifiSection;
    wifiSection.insert(QStringLiteral("ssid"), ssid.toUtf8());
    wifiSection.insert(QStringLiteral("mode"), QStringLiteral("infrastructure"));

    NmSettingsMap settings;
    settings.insert(QStringLiteral("connection"), connSection);
    settings.insert(QStringLiteral("802-11-wireless"), wifiSection);

    if (!password.isEmpty()) {
        QVariantMap secSection;
        secSection.insert(QStringLiteral("key-mgmt"), QStringLiteral("wpa-psk"));
        secSection.insert(QStringLiteral("psk"), password);
        settings.insert(QStringLiteral("802-11-wireless-security"), secSection);
    }

    QDBusReply<QDBusObjectPath> reply = nm.call(
        QStringLiteral("AddAndActivateConnection"),
        QVariant::fromValue(settings),
        QVariant::fromValue(QDBusObjectPath(devPath)),
        QVariant::fromValue(QDBusObjectPath(apPath)));

    if (reply.isValid()) {
        setStatusMessage(QStringLiteral("Connected to ") + ssid, true);
    } else {
        setStatusMessage(reply.error().message(), false);
    }
    QTimer::singleShot(800, this, &NetworkService::refresh);
}

void NetworkService::disconnectWifi() {
    const QString devPath = findWifiDevicePath();
    if (devPath.isEmpty()) return;
    QDBusInterface dev(
        QString::fromLatin1(kNmService),
        devPath,
        QStringLiteral("org.freedesktop.NetworkManager.Device"),
        QDBusConnection::systemBus());
    dev.call(QStringLiteral("Disconnect"));
    setStatusMessage(QStringLiteral("Disconnected"), true);
    QTimer::singleShot(400, this, &NetworkService::refresh);
}

} // namespace lightwm::shell
