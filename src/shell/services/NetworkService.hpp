#pragma once

#include <QMap>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

using NmSettingsMap = QMap<QString, QVariantMap>;
Q_DECLARE_METATYPE(NmSettingsMap)

namespace lightwm::shell {

// Manages Wi-Fi state & networks via org.freedesktop.NetworkManager over QtDBus.
class NetworkService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled WRITE setWifiEnabled NOTIFY stateChanged)
    Q_PROPERTY(QString activeSsid READ activeSsid NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)

public:
    explicit NetworkService(QObject* parent = nullptr);

    bool wifiEnabled() const { return m_wifiEnabled; }
    void setWifiEnabled(bool enabled);

    QString activeSsid() const { return m_activeSsid; }
    QString statusMessage() const { return m_statusMessage; }
    QVariantList networks() const { return m_networks; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void connectToNetwork(const QString& ssid, const QString& password = QString());
    Q_INVOKABLE void disconnectWifi();

signals:
    void stateChanged();
    void networksChanged();
    void statusChanged();
    void connectionStatus(const QString& message, bool ok);

private:
    QString findWifiDevicePath() const;
    QString findSavedConnectionForSsid(const QString& ssid) const;
    void setStatusMessage(const QString& msg, bool ok);

    bool m_wifiEnabled = true;
    QString m_activeSsid;
    QString m_statusMessage;
    QVariantList m_networks;
};

} // namespace lightwm::shell
