#pragma once

#include <QByteArray>
#include <QProcessEnvironment>
#include <QString>

namespace lightwm::ipc {

inline QString socketPath() {
    const auto env = QProcessEnvironment::systemEnvironment();
    QString runtimeDir = env.value(QStringLiteral("XDG_RUNTIME_DIR"), QStringLiteral("/tmp"));
    QString display = env.value(QStringLiteral("WAYLAND_DISPLAY"), QStringLiteral("wayland-0"));
    return runtimeDir + QStringLiteral("/lightwm-") + display + QStringLiteral(".sock");
}

} // namespace lightwm::ipc
