#pragma once

#include <QObject>
#include <QTimer>

namespace lightwm::shell {

// Reads battery capacity and charging status from /sys/class/power_supply.
class BatteryService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(int percentage READ percentage NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)

public:
    explicit BatteryService(QObject* parent = nullptr);

    bool available() const { return m_available; }
    int percentage() const { return m_percentage; }
    bool charging() const { return m_charging; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void refresh();

signals:
    void changed();

private:
    QString m_batteryDir;
    bool m_available = false;
    int m_percentage = 100;
    bool m_charging = false;
    QString m_statusText;
    QTimer m_pollTimer;
};

} // namespace lightwm::shell
