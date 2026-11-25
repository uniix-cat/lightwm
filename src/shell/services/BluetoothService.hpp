#pragma once

#include <QObject>
#include <QVariantList>

namespace lightwm::shell {

// Manages Bluetooth adapter & devices via org.bluez over QtDBus.
class BluetoothService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY stateChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY stateChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)

public:
    explicit BluetoothService(QObject* parent = nullptr);

    bool powered() const { return m_powered; }
    void setPowered(bool powered);

    bool scanning() const { return m_scanning; }
    QVariantList devices() const { return m_devices; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleDiscovery();
    Q_INVOKABLE void toggleDeviceConnection(const QString& objectPath);

signals:
    void stateChanged();
    void devicesChanged();

private:
    QString findAdapterPath() const;

    bool m_powered = false;
    bool m_scanning = false;
    QVariantList m_devices;
};

} // namespace lightwm::shell
