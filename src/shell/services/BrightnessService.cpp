#include "shell/services/BrightnessService.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDir>
#include <QFile>
#include <QtGlobal>

namespace lightwm::shell {

BrightnessService::BrightnessService(QObject* parent) : QObject(parent) {
    QDir dir(QStringLiteral("/sys/class/backlight"));
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (!entries.isEmpty()) {
        m_deviceName = entries.first();
        m_basePath = dir.absoluteFilePath(m_deviceName);
    }
    refresh();
}

void BrightnessService::refresh() {
    if (m_basePath.isEmpty()) return;

    auto readInt = [](const QString& path) -> int {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return 0;
        return f.readAll().trimmed().toInt();
    };

    m_maxBrightness = readInt(m_basePath + QStringLiteral("/max_brightness"));
    const int current = readInt(m_basePath + QStringLiteral("/brightness"));
    if (m_maxBrightness > 0) {
        m_percent = qBound(1, qRound((static_cast<double>(current) * 100.0) / m_maxBrightness), 100);
    }
    emit changed();
}

void BrightnessService::setBrightness(int percent) {
    if (!available()) return;
    const int clamped = qBound(5, percent, 100);
    m_percent = clamped;
    const uint target = static_cast<uint>(qRound((static_cast<double>(clamped) / 100.0) * m_maxBrightness));

    QDBusInterface session(
        QStringLiteral("org.freedesktop.login1"),
        QStringLiteral("/org/freedesktop/login1/session/auto"),
        QStringLiteral("org.freedesktop.login1.Session"),
        QDBusConnection::systemBus());
    if (session.isValid()) {
        session.call(QStringLiteral("SetBrightness"), QStringLiteral("backlight"), m_deviceName, target);
    }
    emit changed();
}

void BrightnessService::stepBrightness(int delta) {
    setBrightness(m_percent + delta);
    emit osdRequested(m_percent);
}

} // namespace lightwm::shell
