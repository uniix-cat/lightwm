#include "settings/SettingsBackend.hpp"
#include "core/IpcProtocol.hpp"
#include "core/ToolkitTheme.hpp"

#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QScreen>
#include <QStandardPaths>
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

QStringList SettingsBackend::availableTerminals() const {
    const QStringList candidates = {
        QStringLiteral("kitty"),
        QStringLiteral("foot"),
        QStringLiteral("alacritty"),
        QStringLiteral("wezterm"),
        QStringLiteral("ghostty"),
        QStringLiteral("konsole"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("xfce4-terminal"),
        QStringLiteral("xterm"),
        QStringLiteral("st"),
        QStringLiteral("urxvt"),
        QStringLiteral("tilix"),
    };
    QStringList found;
    if (!m_cfg.terminal.isEmpty()) {
        found.append(m_cfg.terminal);
    }
    for (const QString& name : candidates) {
        if (!found.contains(name) && !QStandardPaths::findExecutable(name).isEmpty()) {
            found.append(name);
        }
    }
    return found;
}

namespace {

QString labelForShortcutAction(const QString& action, const QString& command) {
    if (action == QStringLiteral("spawn_terminal")) return QStringLiteral("Open Terminal");
    if (action == QStringLiteral("toggle_launcher")) return QStringLiteral("Toggle App Launcher");
    if (action == QStringLiteral("close_focused")) return QStringLiteral("Close Focused Window");
    if (action == QStringLiteral("toggle_float")) return QStringLiteral("Toggle Floating Mode");
    if (action == QStringLiteral("toggle_fullscreen")) return QStringLiteral("Toggle Fullscreen");
    if (action == QStringLiteral("focus_left")) return QStringLiteral("Focus Window Left");
    if (action == QStringLiteral("focus_down")) return QStringLiteral("Focus Window Down");
    if (action == QStringLiteral("focus_up")) return QStringLiteral("Focus Window Up");
    if (action == QStringLiteral("focus_right")) return QStringLiteral("Focus Window Right");
    if (action == QStringLiteral("lock_screen")) return QStringLiteral("Lock Screen");
    if (action == QStringLiteral("screenshot")) return QStringLiteral("Screenshot");
    if (action == QStringLiteral("audio_raise")) return QStringLiteral("Volume Up");
    if (action == QStringLiteral("audio_lower")) return QStringLiteral("Volume Down");
    if (action == QStringLiteral("audio_mute")) return QStringLiteral("Volume Mute");
    if (action == QStringLiteral("brightness_up")) return QStringLiteral("Brightness Up");
    if (action == QStringLiteral("brightness_down")) return QStringLiteral("Brightness Down");
    return QStringLiteral("Run: ") + command;
}

QString formatShortcutCombo(const QString& mods, const QString& key) {
    QString prettyKey = key;
    if (prettyKey.size() == 1) {
        prettyKey = prettyKey.toUpper();
    } else if (prettyKey.startsWith(QStringLiteral("XF86"))) {
        prettyKey = prettyKey.mid(4);
    }
    if (mods.trimmed().isEmpty()) {
        return prettyKey;
    }
    QString prettyMods = mods;
    prettyMods.replace(QLatin1Char('+'), QStringLiteral(" + "));
    return prettyMods + QStringLiteral(" + ") + prettyKey;
}

} // namespace

QVariantList SettingsBackend::shortcuts() const {
    const QList<ShortcutEntry>& list = m_cfg.shortcuts.isEmpty()
        ? ConfigData::defaultShortcuts()
        : m_cfg.shortcuts;
    QVariantList out;
    out.reserve(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const ShortcutEntry& sc = list.at(i);
        QVariantMap item;
        item.insert(QStringLiteral("index"), i);
        item.insert(QStringLiteral("action"), sc.action);
        item.insert(QStringLiteral("label"), labelForShortcutAction(sc.action, sc.command));
        item.insert(QStringLiteral("mods"), sc.mods);
        item.insert(QStringLiteral("key"), sc.key);
        item.insert(QStringLiteral("command"), sc.command);
        item.insert(QStringLiteral("displayCombo"), formatShortcutCombo(sc.mods, sc.key));
        item.insert(QStringLiteral("isCustom"), sc.action == QStringLiteral("custom"));
        out.append(item);
    }
    return out;
}

void SettingsBackend::updateShortcut(int index, const QString& mods, const QString& key) {
    if (m_cfg.shortcuts.isEmpty()) {
        m_cfg.shortcuts = ConfigData::defaultShortcuts();
    }
    if (index < 0 || index >= m_cfg.shortcuts.size() || key.trimmed().isEmpty()) {
        return;
    }
    m_cfg.shortcuts[index].mods = mods.trimmed();
    m_cfg.shortcuts[index].key = key.trimmed();
    applyAndSave();
    emit shortcutsChanged();
}

void SettingsBackend::addCustomShortcut(const QString& mods, const QString& key, const QString& command) {
    if (key.trimmed().isEmpty() || command.trimmed().isEmpty()) {
        return;
    }
    if (m_cfg.shortcuts.isEmpty()) {
        m_cfg.shortcuts = ConfigData::defaultShortcuts();
    }
    m_cfg.shortcuts.append(ShortcutEntry{
        QStringLiteral("custom"),
        mods.trimmed(),
        key.trimmed(),
        command.trimmed(),
    });
    applyAndSave();
    emit shortcutsChanged();
}

void SettingsBackend::removeShortcut(int index) {
    if (index < 0 || index >= m_cfg.shortcuts.size()) {
        return;
    }
    if (m_cfg.shortcuts.at(index).action != QStringLiteral("custom")) {
        return;
    }
    m_cfg.shortcuts.removeAt(index);
    applyAndSave();
    emit shortcutsChanged();
}

void SettingsBackend::resetShortcuts() {
    m_cfg.shortcuts = ConfigData::defaultShortcuts();
    applyAndSave();
    emit shortcutsChanged();
}

QString SettingsBackend::formatModifiers(int qtModifiers) const {
    QStringList parts;
    if (qtModifiers & Qt::MetaModifier) parts << QStringLiteral("Super");
    if (qtModifiers & Qt::ControlModifier) parts << QStringLiteral("Ctrl");
    if (qtModifiers & Qt::AltModifier) parts << QStringLiteral("Alt");
    if (qtModifiers & Qt::ShiftModifier) parts << QStringLiteral("Shift");
    return parts.join(QLatin1Char('+'));
}

QString SettingsBackend::qtKeyToXkbName(int qtKey) const {
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
        return QString(QChar::fromLatin1(static_cast<char>('a' + (qtKey - Qt::Key_A))));
    }
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
        return QString(QChar::fromLatin1(static_cast<char>('0' + (qtKey - Qt::Key_0))));
    }
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12) {
        return QStringLiteral("F%1").arg(1 + (qtKey - Qt::Key_F1));
    }
    switch (qtKey) {
        case Qt::Key_Return: return QStringLiteral("Return");
        case Qt::Key_Enter: return QStringLiteral("KP_Enter");
        case Qt::Key_Space: return QStringLiteral("space");
        case Qt::Key_Tab:
        case Qt::Key_Backtab: return QStringLiteral("Tab");
        case Qt::Key_Backspace: return QStringLiteral("BackSpace");
        case Qt::Key_Delete: return QStringLiteral("Delete");
        case Qt::Key_Insert: return QStringLiteral("Insert");
        case Qt::Key_Home: return QStringLiteral("Home");
        case Qt::Key_End: return QStringLiteral("End");
        case Qt::Key_PageUp: return QStringLiteral("Prior");
        case Qt::Key_PageDown: return QStringLiteral("Next");
        case Qt::Key_Left: return QStringLiteral("Left");
        case Qt::Key_Right: return QStringLiteral("Right");
        case Qt::Key_Up: return QStringLiteral("Up");
        case Qt::Key_Down: return QStringLiteral("Down");
        case Qt::Key_Print: return QStringLiteral("Print");
        case Qt::Key_Minus: return QStringLiteral("minus");
        case Qt::Key_Equal: return QStringLiteral("equal");
        case Qt::Key_BracketLeft: return QStringLiteral("bracketleft");
        case Qt::Key_BracketRight: return QStringLiteral("bracketright");
        case Qt::Key_Semicolon: return QStringLiteral("semicolon");
        case Qt::Key_Apostrophe: return QStringLiteral("apostrophe");
        case Qt::Key_Comma: return QStringLiteral("comma");
        case Qt::Key_Period: return QStringLiteral("period");
        case Qt::Key_Slash: return QStringLiteral("slash");
        case Qt::Key_Backslash: return QStringLiteral("backslash");
        case Qt::Key_QuoteLeft: return QStringLiteral("grave");
        case Qt::Key_VolumeUp: return QStringLiteral("XF86AudioRaiseVolume");
        case Qt::Key_VolumeDown: return QStringLiteral("XF86AudioLowerVolume");
        case Qt::Key_VolumeMute: return QStringLiteral("XF86AudioMute");
        case Qt::Key_MonBrightnessUp: return QStringLiteral("XF86MonBrightnessUp");
        case Qt::Key_MonBrightnessDown: return QStringLiteral("XF86MonBrightnessDown");
        case Qt::Key_MediaPlay: return QStringLiteral("XF86AudioPlay");
        case Qt::Key_MediaNext: return QStringLiteral("XF86AudioNext");
        case Qt::Key_MediaPrevious: return QStringLiteral("XF86AudioPrev");
        default: return {};
    }
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
