#include "settings/SettingsBackend.hpp"
#include "core/IpcProtocol.hpp"

#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QScreen>
#include <QVariantMap>

namespace lightwm::settings {

SettingsBackend::SettingsBackend(QObject* parent) : QObject(parent) {
    m_cfg = ConfigStore::load();
}

void SettingsBackend::applyAndSave() {
    ConfigStore::save(m_cfg);
    notifyWmReload();
    emit changed();
}

void SettingsBackend::notifyWmReload() {
    QLocalSocket sock;
    sock.connectToServer(lightwm::ipc::socketPath());
    if (sock.waitForConnected(200)) {
        const QByteArray payload =
            QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("reload_config")}})
                .toJson(QJsonDocument::Compact) +
            '\n';
        sock.write(payload);
        sock.waitForBytesWritten(200);
    }
}

void SettingsBackend::setWallpaperFromUrl(const QUrl& url) {
    if (url.isLocalFile()) {
        setWallpaperPath(url.toLocalFile());
    } else if (!url.isEmpty()) {
        setWallpaperPath(url.toString());
    }
}

QVariantList SettingsBackend::displays() const {
    QVariantList list;
    for (QScreen* scr : QGuiApplication::screens()) {
        QVariantMap item;
        item.insert(QStringLiteral("name"), scr->name());
        item.insert(QStringLiteral("width"), scr->size().width());
        item.insert(QStringLiteral("height"), scr->size().height());
        item.insert(QStringLiteral("refresh"), qRound(scr->refreshRate()));
        item.insert(QStringLiteral("scale"), scr->devicePixelRatio());
        list.append(item);
    }
    return list;
}

} // namespace lightwm::settings
