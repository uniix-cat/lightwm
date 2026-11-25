#include "shell/WmClient.hpp"
#include "core/IpcProtocol.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace lightwm::shell {

WmClient::WmClient(QObject* parent) : QObject(parent) {
    connect(&m_socket, &QLocalSocket::connected, this, [this]() {
        m_connected = true;
        m_reconnectTimer.stop();
        emit connectedChanged();
    });

    connect(&m_socket, &QLocalSocket::disconnected, this, [this]() {
        m_connected = false;
        emit connectedChanged();
        m_reconnectTimer.start(1000);
    });

    connect(&m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        if (!m_reconnectTimer.isActive()) {
            m_reconnectTimer.start(1000);
        }
    });

    connect(&m_socket, &QLocalSocket::readyRead, this, &WmClient::onReadyRead);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WmClient::tryConnect);

    tryConnect();
}

void WmClient::tryConnect() {
    if (m_socket.state() == QLocalSocket::UnconnectedState) {
        m_socket.connectToServer(lightwm::ipc::socketPath());
    }
}

void WmClient::onReadyRead() {
    m_buffer.append(m_socket.readAll());
    int idx = -1;
    while ((idx = m_buffer.indexOf('\n')) != -1) {
        const QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);
        if (line.isEmpty()) continue;

        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        const QJsonObject obj = doc.object();

        if (obj.contains(QStringLiteral("event"))) {
            const QString ev = obj.value(QStringLiteral("event")).toString();
            if (ev == QStringLiteral("toggle_launcher")) emit toggleLauncherRequested();
            else if (ev == QStringLiteral("lock_screen")) emit lockScreenRequested();
            else if (ev == QStringLiteral("screenshot")) emit screenshotRequested();
            else if (ev == QStringLiteral("audio_step")) emit audioStepRequested(obj.value(QStringLiteral("delta")).toInt(5));
            else if (ev == QStringLiteral("audio_mute_toggle")) emit audioMuteToggleRequested();
            else if (ev == QStringLiteral("brightness_step")) emit brightnessStepRequested(obj.value(QStringLiteral("delta")).toInt(5));
            continue;
        }

        if (obj.value(QStringLiteral("type")).toString() == QStringLiteral("state")) {
            m_workspaceCount = obj.value(QStringLiteral("workspaceCount")).toInt(9);
            m_activeWorkspace = obj.value(QStringLiteral("activeWorkspace")).toInt(1);
            m_activeTitle = obj.value(QStringLiteral("activeTitle")).toString();
            m_activeAppId = obj.value(QStringLiteral("activeAppId")).toString();
            m_keyboardLayout = obj.value(QStringLiteral("keyboardLayout")).toString(QStringLiteral("us"));
            m_isFloating = obj.value(QStringLiteral("isFloating")).toBool(false);

            m_occupiedWorkspaces.clear();
            const QJsonArray occ = obj.value(QStringLiteral("occupiedWorkspaces")).toArray();
            for (const auto& v : occ) {
                m_occupiedWorkspaces.append(v.toBool(false));
            }
            emit stateChanged();
        }
    }
}

void WmClient::sendJson(const QJsonObject& obj) {
    if (m_socket.state() != QLocalSocket::ConnectedState) return;
    const QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    m_socket.write(data);
    m_socket.flush();
}

void WmClient::switchWorkspace(int ws) {
    sendJson({{QStringLiteral("cmd"), QStringLiteral("switch_workspace")},
              {QStringLiteral("workspace"), ws}});
}

void WmClient::toggleFloat() {
    sendJson({{QStringLiteral("cmd"), QStringLiteral("toggle_float")}});
}

void WmClient::closeFocused() {
    sendJson({{QStringLiteral("cmd"), QStringLiteral("close_focused")}});
}

void WmClient::reloadConfig() {
    sendJson({{QStringLiteral("cmd"), QStringLiteral("reload_config")}});
}

void WmClient::spawnTerminal() {
    sendJson({{QStringLiteral("cmd"), QStringLiteral("spawn_terminal")}});
}

} // namespace lightwm::shell
