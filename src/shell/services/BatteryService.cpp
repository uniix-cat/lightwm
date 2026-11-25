#include "shell/services/BatteryService.hpp"

#include <QDir>
#include <QFile>
#include <QtGlobal>

namespace lightwm::shell {

namespace {

QString readTrimmed(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}

} // namespace

BatteryService::BatteryService(QObject* parent) : QObject(parent) {
    QDir ps(QStringLiteral("/sys/class/power_supply"));
    const QStringList entries = ps.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& name : entries) {
        const QString dir = ps.absoluteFilePath(name);
        if (readTrimmed(dir + QStringLiteral("/type")) == QStringLiteral("Battery")) {
            m_batteryDir = dir;
            m_available = true;
            break;
        }
    }

    refresh();
    connect(&m_pollTimer, &QTimer::timeout, this, &BatteryService::refresh);
    m_pollTimer.start(15000);
}

void BatteryService::refresh() {
    if (!m_available || m_batteryDir.isEmpty()) return;

    const int cap = qBound(0, readTrimmed(m_batteryDir + QStringLiteral("/capacity")).toInt(), 100);
    const QString status = readTrimmed(m_batteryDir + QStringLiteral("/status"));
    const bool isCharging = (status == QStringLiteral("Charging") || status == QStringLiteral("Full"));

    if (cap != m_percentage || isCharging != m_charging || status != m_statusText) {
        m_percentage = cap;
        m_charging = isCharging;
        m_statusText = status;
        emit changed();
    }
}

} // namespace lightwm::shell
