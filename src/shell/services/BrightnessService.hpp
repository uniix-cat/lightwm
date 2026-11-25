#pragma once

#include <QObject>

namespace lightwm::shell {

// Reads /sys/class/backlight and sets brightness via systemd-logind D-Bus.
class BrightnessService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY changed)

public:
    explicit BrightnessService(QObject* parent = nullptr);

    bool available() const { return !m_deviceName.isEmpty() && m_maxBrightness > 0; }
    int brightness() const { return m_percent; }
    void setBrightness(int percent);

    Q_INVOKABLE void stepBrightness(int delta);
    Q_INVOKABLE void refresh();

signals:
    void changed();
    void osdRequested(int brightness);

private:
    QString m_deviceName;
    QString m_basePath;
    int m_maxBrightness = 0;
    int m_percent = 100;
};

} // namespace lightwm::shell
