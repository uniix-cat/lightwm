#include "core/Config.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace lightwm {

QList<ShortcutEntry> ConfigData::defaultShortcuts() {
    return {
        {QStringLiteral("spawn_terminal"), QStringLiteral("Super"), QStringLiteral("q"), {}},
        {QStringLiteral("toggle_launcher"), QStringLiteral("Super"), QStringLiteral("r"), {}},
        {QStringLiteral("close_focused"), QStringLiteral("Super"), QStringLiteral("c"), {}},
        {QStringLiteral("toggle_float"), QStringLiteral("Super"), QStringLiteral("v"), {}},
        {QStringLiteral("toggle_fullscreen"), QStringLiteral("Super"), QStringLiteral("f"), {}},
        {QStringLiteral("focus_left"), QStringLiteral("Super"), QStringLiteral("h"), {}},
        {QStringLiteral("focus_down"), QStringLiteral("Super"), QStringLiteral("j"), {}},
        {QStringLiteral("focus_up"), QStringLiteral("Super"), QStringLiteral("k"), {}},
        {QStringLiteral("focus_right"), QStringLiteral("Super"), QStringLiteral("l"), {}},
        {QStringLiteral("lock_screen"), QStringLiteral("Super+Shift"), QStringLiteral("l"), {}},
        {QStringLiteral("screenshot"), QString(), QStringLiteral("Print"), {}},
        {QStringLiteral("audio_raise"), QString(), QStringLiteral("XF86AudioRaiseVolume"), {}},
        {QStringLiteral("audio_lower"), QString(), QStringLiteral("XF86AudioLowerVolume"), {}},
        {QStringLiteral("audio_mute"), QString(), QStringLiteral("XF86AudioMute"), {}},
        {QStringLiteral("brightness_up"), QString(), QStringLiteral("XF86MonBrightnessUp"), {}},
        {QStringLiteral("brightness_down"), QString(), QStringLiteral("XF86MonBrightnessDown"), {}},
    };
}

QString ConfigStore::configFilePath() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return base + QStringLiteral("/lightwm/config.json");
}

ConfigData ConfigStore::load() {
    ConfigData cfg;
    QFile file(configFilePath());
    if (!file.exists()) {
        save(cfg);
        return cfg;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return cfg;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return cfg;
    }

    const QJsonObject root = doc.object();
    cfg.themeName = root.value(QStringLiteral("theme")).toString(cfg.themeName);
    cfg.gapsInner = root.value(QStringLiteral("gapsInner")).toInt(cfg.gapsInner);
    cfg.gapsOuter = root.value(QStringLiteral("gapsOuter")).toInt(cfg.gapsOuter);
    cfg.borderWidth = root.value(QStringLiteral("borderWidth")).toInt(cfg.borderWidth);
    cfg.cornerRadius = root.value(QStringLiteral("cornerRadius")).toInt(cfg.cornerRadius);
    cfg.fontFamily = root.value(QStringLiteral("fontFamily")).toString(cfg.fontFamily);
    cfg.iconFontFamily = root.value(QStringLiteral("iconFontFamily")).toString(cfg.iconFontFamily);
    cfg.barHeight = root.value(QStringLiteral("barHeight")).toInt(cfg.barHeight);
    cfg.wallpaperPath = root.value(QStringLiteral("wallpaperPath")).toString(cfg.wallpaperPath);
    cfg.wallpaperMode = root.value(QStringLiteral("wallpaperMode")).toString(cfg.wallpaperMode);
    cfg.blurEnabled = root.value(QStringLiteral("blurEnabled")).toBool(cfg.blurEnabled);
    cfg.blurStrength = qBound(0, root.value(QStringLiteral("blurStrength")).toInt(cfg.blurStrength), 64);
    cfg.glassOpacity = qBound(0.35, root.value(QStringLiteral("glassOpacity")).toDouble(cfg.glassOpacity), 0.95);

    cfg.workspaceCount = qBound(1, root.value(QStringLiteral("workspaceCount")).toInt(cfg.workspaceCount), 12);
    cfg.dwindleSplitRatio = qBound(0.2, root.value(QStringLiteral("dwindleSplitRatio")).toDouble(cfg.dwindleSplitRatio), 0.8);

    cfg.keyboardLayouts = root.value(QStringLiteral("keyboardLayouts")).toString(cfg.keyboardLayouts);
    cfg.keyboardOptions = root.value(QStringLiteral("keyboardOptions")).toString(cfg.keyboardOptions);
    cfg.tapToClick = root.value(QStringLiteral("tapToClick")).toBool(cfg.tapToClick);
    cfg.naturalScroll = root.value(QStringLiteral("naturalScroll")).toBool(cfg.naturalScroll);
    cfg.pointerSpeed = qBound(-1.0, root.value(QStringLiteral("pointerSpeed")).toDouble(cfg.pointerSpeed), 1.0);

    cfg.terminal = root.value(QStringLiteral("terminal")).toString(cfg.terminal);

    const QJsonValue scalesVal = root.value(QStringLiteral("outputScales"));
    if (scalesVal.isObject()) {
        const QJsonObject scalesObj = scalesVal.toObject();
        for (auto it = scalesObj.constBegin(); it != scalesObj.constEnd(); ++it) {
            if (!it.key().isEmpty() && it.value().isDouble()) {
                const double s = it.value().toDouble(1.0);
                if (s > 0.0) {
                    cfg.outputScales.insert(it.key(), qBound(0.5, s, 4.0));
                }
            }
        }
    }

    const QString defaultScheme = (cfg.themeName == QStringLiteral("cream"))
        ? QStringLiteral("prefer-light")
        : QStringLiteral("prefer-dark");
    cfg.colorScheme = root.value(QStringLiteral("colorScheme")).toString(defaultScheme);
    cfg.gtkTheme = root.value(QStringLiteral("gtkTheme")).toString();
    cfg.iconTheme = root.value(QStringLiteral("iconTheme")).toString();
    cfg.cursorTheme = root.value(QStringLiteral("cursorTheme")).toString();
    cfg.cursorSize = qBound(16, root.value(QStringLiteral("cursorSize")).toInt(cfg.cursorSize), 64);
    cfg.qtPlatformTheme = root.value(QStringLiteral("qtPlatformTheme")).toString(cfg.qtPlatformTheme);
    cfg.qtStyle = root.value(QStringLiteral("qtStyle")).toString();

    const QJsonValue scVal = root.value(QStringLiteral("shortcuts"));
    if (scVal.isArray()) {
        for (const QJsonValue& v : scVal.toArray()) {
            if (!v.isObject()) continue;
            const QJsonObject o = v.toObject();
            ShortcutEntry entry;
            entry.action = o.value(QStringLiteral("action")).toString();
            entry.mods = o.value(QStringLiteral("mods")).toString();
            entry.key = o.value(QStringLiteral("key")).toString();
            entry.command = o.value(QStringLiteral("command")).toString();
            if (!entry.action.isEmpty() && !entry.key.isEmpty()) {
                cfg.shortcuts.append(entry);
            }
        }
    } else {
        cfg.shortcuts = ConfigData::defaultShortcuts();
    }
    return cfg;
}

bool ConfigStore::save(const ConfigData& cfg) {
    const QString path = configFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject root;
    root.insert(QStringLiteral("theme"), cfg.themeName);
    root.insert(QStringLiteral("gapsInner"), cfg.gapsInner);
    root.insert(QStringLiteral("gapsOuter"), cfg.gapsOuter);
    root.insert(QStringLiteral("borderWidth"), cfg.borderWidth);
    root.insert(QStringLiteral("cornerRadius"), cfg.cornerRadius);
    root.insert(QStringLiteral("fontFamily"), cfg.fontFamily);
    root.insert(QStringLiteral("iconFontFamily"), cfg.iconFontFamily);
    root.insert(QStringLiteral("barHeight"), cfg.barHeight);
    root.insert(QStringLiteral("wallpaperPath"), cfg.wallpaperPath);
    root.insert(QStringLiteral("wallpaperMode"), cfg.wallpaperMode);
    root.insert(QStringLiteral("blurEnabled"), cfg.blurEnabled);
    root.insert(QStringLiteral("blurStrength"), cfg.blurStrength);
    root.insert(QStringLiteral("glassOpacity"), cfg.glassOpacity);

    root.insert(QStringLiteral("workspaceCount"), cfg.workspaceCount);
    root.insert(QStringLiteral("dwindleSplitRatio"), cfg.dwindleSplitRatio);

    root.insert(QStringLiteral("keyboardLayouts"), cfg.keyboardLayouts);
    root.insert(QStringLiteral("keyboardOptions"), cfg.keyboardOptions);
    root.insert(QStringLiteral("tapToClick"), cfg.tapToClick);
    root.insert(QStringLiteral("naturalScroll"), cfg.naturalScroll);
    root.insert(QStringLiteral("pointerSpeed"), cfg.pointerSpeed);

    root.insert(QStringLiteral("terminal"), cfg.terminal);

    QJsonObject scalesObj;
    for (auto it = cfg.outputScales.constBegin(); it != cfg.outputScales.constEnd(); ++it) {
        if (!it.key().isEmpty() && it.value() > 0.0) {
            scalesObj.insert(it.key(), qBound(0.5, it.value(), 4.0));
        }
    }
    root.insert(QStringLiteral("outputScales"), scalesObj);

    root.insert(QStringLiteral("colorScheme"), cfg.colorScheme);
    root.insert(QStringLiteral("gtkTheme"), cfg.gtkTheme);
    root.insert(QStringLiteral("iconTheme"), cfg.iconTheme);
    root.insert(QStringLiteral("cursorTheme"), cfg.cursorTheme);
    root.insert(QStringLiteral("cursorSize"), cfg.cursorSize);
    root.insert(QStringLiteral("qtPlatformTheme"), cfg.qtPlatformTheme);
    root.insert(QStringLiteral("qtStyle"), cfg.qtStyle);

    QJsonArray scArr;
    const QList<ShortcutEntry>& shortcutsToSave = cfg.shortcuts.isEmpty()
        ? ConfigData::defaultShortcuts()
        : cfg.shortcuts;
    for (const ShortcutEntry& entry : shortcutsToSave) {
        QJsonObject o;
        o.insert(QStringLiteral("action"), entry.action);
        o.insert(QStringLiteral("mods"), entry.mods);
        o.insert(QStringLiteral("key"), entry.key);
        if (!entry.command.isEmpty()) {
            o.insert(QStringLiteral("command"), entry.command);
        }
        scArr.append(o);
    }
    root.insert(QStringLiteral("shortcuts"), scArr);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace lightwm
