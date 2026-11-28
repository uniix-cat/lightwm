#include "settings/SettingsBackend.hpp"
#include "core/IpcProtocol.hpp"
#include "core/ToolkitTheme.hpp"

#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QScreen>
#include <QVariantMap>
#include <cmath>

namespace lightwm::settings {

SettingsBackend::SettingsBackend(QObject* parent) : QObject(parent) {
    m_cfg = ConfigStore::load();
    ToolkitTheme::populateSystemDefaults(m_cfg);
    refreshToolkitThemes();

    connect(qGuiApp, &QGuiApplication::screenAdded, this, &SettingsBackend::displaysChanged);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &SettingsBackend::displaysChanged);

    connect(&m_ipcSocket, &QLocalSocket::connected, this, [this]() {
        m_reconnectTimer.stop();
    });
    connect(&m_ipcSocket, &QLocalSocket::disconnected, this, [this]() {
        m_reconnectTimer.start(1000);
    });
    connect(&m_ipcSocket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        if (!m_reconnectTimer.isActive()) {
            m_reconnectTimer.start(1000);
        }
    });
    connect(&m_ipcSocket, &QLocalSocket::readyRead, this, &SettingsBackend::onIpcReadyRead);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &SettingsBackend::tryConnectIpc);

    tryConnectIpc();
}

void SettingsBackend::tryConnectIpc() {
    if (m_ipcSocket.state() == QLocalSocket::UnconnectedState) {
        m_ipcSocket.connectToServer(lightwm::ipc::socketPath());
    }
}

void SettingsBackend::onIpcReadyRead() {
    m_ipcBuffer.append(m_ipcSocket.readAll());
    int idx = -1;
    while ((idx = m_ipcBuffer.indexOf('\n')) != -1) {
        const QByteArray line = m_ipcBuffer.left(idx);
        m_ipcBuffer.remove(0, idx + 1);
        if (line.isEmpty()) continue;

        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        const QJsonObject obj = doc.object();
        if (obj.value(QStringLiteral("type")).toString() != QStringLiteral("state")) continue;
        if (!obj.contains(QStringLiteral("outputs"))) continue;

        QVariantList nextOutputs;
        const QJsonArray outputs = obj.value(QStringLiteral("outputs")).toArray();
        for (const QJsonValue& v : outputs) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            if (!o.value(QStringLiteral("enabled")).toBool(true)) continue;

            const QString name = o.value(QStringLiteral("name")).toString();
            const int w = o.value(QStringLiteral("width")).toInt();
            const int h = o.value(QStringLiteral("height")).toInt();
            const double scale = o.value(QStringLiteral("scale")).toDouble(1.0);
            const int lw = o.value(QStringLiteral("logicalWidth")).toInt(qRound(w / qMax(0.5, scale)));
            const int lh = o.value(QStringLiteral("logicalHeight")).toInt(qRound(h / qMax(0.5, scale)));

            QVariantMap item;
            item.insert(QStringLiteral("name"), name);
            item.insert(QStringLiteral("description"), o.value(QStringLiteral("description")).toString());
            item.insert(QStringLiteral("width"), w);
            item.insert(QStringLiteral("height"), h);
            item.insert(QStringLiteral("logicalWidth"), lw);
            item.insert(QStringLiteral("logicalHeight"), lh);
            item.insert(QStringLiteral("refresh"), o.value(QStringLiteral("refresh")).toInt(60));
            item.insert(QStringLiteral("scale"), scale);
            nextOutputs.append(item);
        }

        if (nextOutputs != m_ipcOutputs) {
            m_ipcOutputs = nextOutputs;
            emit displaysChanged();
        }
    }
}

void SettingsBackend::applyAndSave() {
    ConfigStore::save(m_cfg);
    notifyWmReload();
    emit changed();
}

void SettingsBackend::applyToolkitAndSave() {
    ToolkitTheme::apply(m_cfg);
    applyAndSave();
}

QString SettingsBackend::resolvedQtPlatformTheme() const {
    return ToolkitTheme::resolvedQtPlatformTheme(m_cfg);
}

void SettingsBackend::refreshToolkitThemes() {
    m_gtkThemes = ToolkitTheme::availableGtkThemes();
    m_iconThemes = ToolkitTheme::availableIconThemes();
    m_cursorThemes = ToolkitTheme::availableCursorThemes();
    m_qtStyles = QStringList{QString()} + ToolkitTheme::availableQtStyles();
    m_qtPlatformThemes = ToolkitTheme::availableQtPlatformThemes();
    emit toolkitListsChanged();
}

void SettingsBackend::notifyWmReload() {
    const QByteArray payload =
        QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("reload_config")}})
            .toJson(QJsonDocument::Compact) +
        '\n';
    if (m_ipcSocket.state() == QLocalSocket::ConnectedState) {
        m_ipcSocket.write(payload);
        m_ipcSocket.flush();
        return;
    }

    QLocalSocket sock;
    sock.connectToServer(lightwm::ipc::socketPath());
    if (sock.waitForConnected(200)) {
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

void SettingsBackend::setOutputScale(const QString& outputName, double scale) {
    if (outputName.isEmpty()) return;
    const double clamped = qBound(0.5, std::round(scale * 100.0) / 100.0, 4.0);
    m_cfg.outputScales.insert(outputName, clamped);
    applyAndSave();
    emit displaysChanged();
}

QVariantList SettingsBackend::displays() const {
    if (!m_ipcOutputs.isEmpty()) {
        QVariantList merged;
        merged.reserve(m_ipcOutputs.size());
        for (const QVariant& entry : m_ipcOutputs) {
            QVariantMap item = entry.toMap();
            const QString name = item.value(QStringLiteral("name")).toString();
            const double liveScale = item.value(QStringLiteral("scale")).toDouble();
            const double configured = m_cfg.outputScales.value(name, liveScale > 0.0 ? liveScale : 1.0);
            item.insert(QStringLiteral("configuredScale"), configured);
            merged.append(item);
        }
        return merged;
    }

    QVariantList list;
    for (QScreen* scr : QGuiApplication::screens()) {
        const QString name = scr->name();
        const double liveScale = scr->devicePixelRatio();
        const double configured = m_cfg.outputScales.value(name, liveScale);

        QVariantMap item;
        item.insert(QStringLiteral("name"), name);
        item.insert(QStringLiteral("description"), scr->model());
        item.insert(QStringLiteral("width"), scr->size().width());
        item.insert(QStringLiteral("height"), scr->size().height());
        item.insert(QStringLiteral("logicalWidth"), scr->size().width());
        item.insert(QStringLiteral("logicalHeight"), scr->size().height());
        item.insert(QStringLiteral("refresh"), qRound(scr->refreshRate()));
        item.insert(QStringLiteral("scale"), liveScale);
        item.insert(QStringLiteral("configuredScale"), configured);
        list.append(item);
    }
    return list;
}

} // namespace lightwm::settings
