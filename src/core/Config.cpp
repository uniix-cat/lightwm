#include "core/Config.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace lightwm {

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

    cfg.workspaceCount = qBound(1, root.value(QStringLiteral("workspaceCount")).toInt(cfg.workspaceCount), 12);
    cfg.dwindleSplitRatio = qBound(0.2, root.value(QStringLiteral("dwindleSplitRatio")).toDouble(cfg.dwindleSplitRatio), 0.8);

    cfg.keyboardLayouts = root.value(QStringLiteral("keyboardLayouts")).toString(cfg.keyboardLayouts);
    cfg.keyboardOptions = root.value(QStringLiteral("keyboardOptions")).toString(cfg.keyboardOptions);
    cfg.tapToClick = root.value(QStringLiteral("tapToClick")).toBool(cfg.tapToClick);
    cfg.naturalScroll = root.value(QStringLiteral("naturalScroll")).toBool(cfg.naturalScroll);
    cfg.pointerSpeed = qBound(-1.0, root.value(QStringLiteral("pointerSpeed")).toDouble(cfg.pointerSpeed), 1.0);

    cfg.terminal = root.value(QStringLiteral("terminal")).toString(cfg.terminal);
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

    root.insert(QStringLiteral("workspaceCount"), cfg.workspaceCount);
    root.insert(QStringLiteral("dwindleSplitRatio"), cfg.dwindleSplitRatio);

    root.insert(QStringLiteral("keyboardLayouts"), cfg.keyboardLayouts);
    root.insert(QStringLiteral("keyboardOptions"), cfg.keyboardOptions);
    root.insert(QStringLiteral("tapToClick"), cfg.tapToClick);
    root.insert(QStringLiteral("naturalScroll"), cfg.naturalScroll);
    root.insert(QStringLiteral("pointerSpeed"), cfg.pointerSpeed);

    root.insert(QStringLiteral("terminal"), cfg.terminal);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace lightwm
