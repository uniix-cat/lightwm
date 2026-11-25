#pragma once

#include <QLocalSocket>
#include <QObject>
#include <QTimer>
#include <QVariantList>

namespace lightwm::shell {

class WmClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(int workspaceCount READ workspaceCount NOTIFY stateChanged)
    Q_PROPERTY(int activeWorkspace READ activeWorkspace NOTIFY stateChanged)
    Q_PROPERTY(QVariantList occupiedWorkspaces READ occupiedWorkspaces NOTIFY stateChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY stateChanged)
    Q_PROPERTY(QString activeAppId READ activeAppId NOTIFY stateChanged)
    Q_PROPERTY(QString keyboardLayout READ keyboardLayout NOTIFY stateChanged)
    Q_PROPERTY(bool isFloating READ isFloating NOTIFY stateChanged)

public:
    explicit WmClient(QObject* parent = nullptr);

    bool connected() const { return m_connected; }
    int workspaceCount() const { return m_workspaceCount; }
    int activeWorkspace() const { return m_activeWorkspace; }
    QVariantList occupiedWorkspaces() const { return m_occupiedWorkspaces; }
    QString activeTitle() const { return m_activeTitle; }
    QString activeAppId() const { return m_activeAppId; }
    QString keyboardLayout() const { return m_keyboardLayout; }
    bool isFloating() const { return m_isFloating; }

    Q_INVOKABLE void switchWorkspace(int ws);
    Q_INVOKABLE void toggleFloat();
    Q_INVOKABLE void closeFocused();
    Q_INVOKABLE void reloadConfig();
    Q_INVOKABLE void spawnTerminal();

signals:
    void connectedChanged();
    void stateChanged();
    void toggleLauncherRequested();
    void lockScreenRequested();
    void screenshotRequested();
    void audioStepRequested(int delta);
    void audioMuteToggleRequested();
    void brightnessStepRequested(int delta);

private slots:
    void tryConnect();
    void onReadyRead();

private:
    void sendJson(const QJsonObject& obj);

    QLocalSocket m_socket;
    QTimer m_reconnectTimer;
    QByteArray m_buffer;
    bool m_connected = false;

    int m_workspaceCount = 9;
    int m_activeWorkspace = 1;
    QVariantList m_occupiedWorkspaces;
    QString m_activeTitle;
    QString m_activeAppId;
    QString m_keyboardLayout = QStringLiteral("us");
    bool m_isFloating = false;
};

} // namespace lightwm::shell
