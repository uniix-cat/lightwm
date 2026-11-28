#include "core/ToolkitTheme.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QPluginLoader>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <algorithm>
#include <cstdlib>

namespace lightwm {

namespace {

QStringList searchDataDirs(const QString& subdir) {
    QStringList dirs;
    const QString home = QDir::homePath();
    if (subdir == QStringLiteral("themes")) {
        dirs << home + QStringLiteral("/.themes");
    } else if (subdir == QStringLiteral("icons")) {
        dirs << home + QStringLiteral("/.icons");
    }
    const QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!dataHome.isEmpty()) {
        dirs << dataHome + QLatin1Char('/') + subdir;
    }
    const QStringList sysDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString& base : sysDirs) {
        const QString p = base + QLatin1Char('/') + subdir;
        if (!dirs.contains(p)) {
            dirs << p;
        }
    }
    const QString fallback = QStringLiteral("/usr/share/") + subdir;
    if (!dirs.contains(fallback)) {
        dirs << fallback;
    }
    return dirs;
}

QStringList sortUniqueCaseInsensitive(const QSet<QString>& items) {
    QStringList list = items.values();
    std::sort(list.begin(), list.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return list;
}

bool hasIconDirectories(const QString& indexThemePath) {
    QFile f(indexThemePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QStringLiteral("Directories="))) {
            return !line.mid(12).trimmed().isEmpty();
        }
    }
    return false;
}

// Updates or inserts keys inside a single [Section] in an INI file, preserving all other content.
void updateIniKeys(const QString& filePath, const QString& sectionName, const QMap<QString, QString>& updates) {
    if (updates.isEmpty()) return;
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QStringList lines;
    QFile inFile(filePath);
    if (inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&inFile);
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }
        inFile.close();
    }

    const QString targetHeader = QStringLiteral("[") + sectionName + QStringLiteral("]");
    QSet<QString> handled;
    QStringList out;
    out.reserve(lines.size() + updates.size() + 2);

    bool inTarget = false;
    bool foundSection = false;

    auto flushRemaining = [&]() {
        for (auto it = updates.constBegin(); it != updates.constEnd(); ++it) {
            if (!handled.contains(it.key())) {
                out.append(it.key() + QLatin1Char('=') + it.value());
                handled.insert(it.key());
            }
        }
    };

    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
            if (inTarget) {
                flushRemaining();
                inTarget = false;
            }
            if (trimmed == targetHeader) {
                inTarget = true;
                foundSection = true;
            }
            out.append(line);
            continue;
        }

        if (inTarget && !trimmed.isEmpty() && !trimmed.startsWith(QLatin1Char('#')) && !trimmed.startsWith(QLatin1Char(';'))) {
            const int eq = trimmed.indexOf(QLatin1Char('='));
            if (eq > 0) {
                const QString key = trimmed.left(eq).trimmed();
                if (updates.contains(key)) {
                    out.append(key + QLatin1Char('=') + updates.value(key));
                    handled.insert(key);
                    continue;
                }
            }
        }
        out.append(line);
    }

    if (inTarget) {
        flushRemaining();
    } else if (!foundSection) {
        if (!out.isEmpty() && !out.last().isEmpty()) {
            out.append(QString());
        }
        out.append(targetHeader);
        flushRemaining();
    }

    QFile outFile(filePath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream ts(&outFile);
        for (const QString& l : out) {
            ts << l << '\n';
        }
    }
}

QString readIniKey(const QString& filePath, const QString& sectionName, const QString& keyName) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    const QString targetHeader = QStringLiteral("[") + sectionName + QStringLiteral("]");
    bool inTarget = false;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString trimmed = in.readLine().trimmed();
        if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
            inTarget = (trimmed == targetHeader);
            continue;
        }
        if (inTarget) {
            const int eq = trimmed.indexOf(QLatin1Char('='));
            if (eq > 0 && trimmed.left(eq).trimmed() == keyName) {
                return trimmed.mid(eq + 1).trimmed();
            }
        }
    }
    return {};
}

QString queryGsetting(const QString& key) {
    if (QStandardPaths::findExecutable(QStringLiteral("gsettings")).isEmpty()) {
        return {};
    }
    QProcess proc;
    proc.start(QStringLiteral("gsettings"), {QStringLiteral("get"), QStringLiteral("org.gnome.desktop.interface"), key});
    if (!proc.waitForFinished(400) || proc.exitCode() != 0) {
        return {};
    }
    QString val = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (val.startsWith(QLatin1Char('\'')) && val.endsWith(QLatin1Char('\'')) && val.size() >= 2) {
        val = val.mid(1, val.size() - 2);
    }
    return val;
}

void setGsetting(const QString& key, const QString& value) {
    if (value.isEmpty() || QStandardPaths::findExecutable(QStringLiteral("gsettings")).isEmpty()) {
        return;
    }
    QProcess::startDetached(
        QStringLiteral("gsettings"),
        {QStringLiteral("set"), QStringLiteral("org.gnome.desktop.interface"), key, value});
}

QStringList scanQtPluginKeys(const QString& subdir, const QStringList& ignoreKeys = {}) {
    QSet<QString> found;
    const QString pluginsDir = QLibraryInfo::path(QLibraryInfo::PluginsPath) + QLatin1Char('/') + subdir;
    QDir dir(pluginsDir);
    if (dir.exists()) {
        for (const QString& fileName : dir.entryList(QDir::Files)) {
            QPluginLoader loader(dir.absoluteFilePath(fileName));
            const QJsonArray keys = loader.metaData().value(QStringLiteral("MetaData")).toObject().value(QStringLiteral("Keys")).toArray();
            for (const QJsonValue& k : keys) {
                const QString name = k.toString();
                if (!name.isEmpty() && !ignoreKeys.contains(name)) {
                    found.insert(name);
                }
            }
        }
    }
    return sortUniqueCaseInsensitive(found);
}

} // namespace

QStringList ToolkitTheme::availableGtkThemes() {
    QSet<QString> themes;
    for (const QString& base : searchDataDirs(QStringLiteral("themes"))) {
        QDir dir(base);
        if (!dir.exists()) continue;
        for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString p = fi.absoluteFilePath();
            if (QDir(p + QStringLiteral("/gtk-3.0")).exists() ||
                QDir(p + QStringLiteral("/gtk-4.0")).exists() ||
                QFileInfo::exists(p + QStringLiteral("/index.theme"))) {
                themes.insert(fi.fileName());
            }
        }
    }
    return sortUniqueCaseInsensitive(themes);
}

QStringList ToolkitTheme::availableIconThemes() {
    QSet<QString> themes;
    for (const QString& base : searchDataDirs(QStringLiteral("icons"))) {
        QDir dir(base);
        if (!dir.exists()) continue;
        for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString name = fi.fileName();
            if (name == QStringLiteral("default") || name == QStringLiteral("hicolor") || name == QStringLiteral("locolor")) {
                continue;
            }
            const QString idx = fi.absoluteFilePath() + QStringLiteral("/index.theme");
            if (QFileInfo::exists(idx) && hasIconDirectories(idx)) {
                themes.insert(name);
            }
        }
    }
    return sortUniqueCaseInsensitive(themes);
}

QStringList ToolkitTheme::availableCursorThemes() {
    QSet<QString> themes;
    for (const QString& base : searchDataDirs(QStringLiteral("icons"))) {
        QDir dir(base);
        if (!dir.exists()) continue;
        for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (fi.fileName() == QStringLiteral("default")) continue;
            if (QDir(fi.absoluteFilePath() + QStringLiteral("/cursors")).exists()) {
                themes.insert(fi.fileName());
            }
        }
    }
    return sortUniqueCaseInsensitive(themes);
}

QStringList ToolkitTheme::availableQtStyles() {
    QSet<QString> styles{QStringLiteral("Fusion"), QStringLiteral("Windows")};
    for (const QString& k : scanQtPluginKeys(QStringLiteral("styles"))) {
        styles.insert(k);
    }
    return sortUniqueCaseInsensitive(styles);
}

QStringList ToolkitTheme::availableQtPlatformThemes() {
    QStringList list{QStringLiteral("auto")};
    const QStringList discovered = scanQtPluginKeys(
        QStringLiteral("platformthemes"),
        {QStringLiteral("flatpak"), QStringLiteral("snap")});
    for (const QString& k : discovered) {
        if (!list.contains(k)) {
            list.append(k);
        }
    }
    return list;
}

void ToolkitTheme::populateSystemDefaults(ConfigData& cfg) {
    const QString cfgBase = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString gtk3Ini = cfgBase + QStringLiteral("/gtk-3.0/settings.ini");

    if (cfg.gtkTheme.isEmpty()) {
        cfg.gtkTheme = queryGsetting(QStringLiteral("gtk-theme"));
        if (cfg.gtkTheme.isEmpty()) {
            cfg.gtkTheme = readIniKey(gtk3Ini, QStringLiteral("Settings"), QStringLiteral("gtk-theme-name"));
        }
    }
    if (cfg.iconTheme.isEmpty()) {
        cfg.iconTheme = queryGsetting(QStringLiteral("icon-theme"));
        if (cfg.iconTheme.isEmpty()) {
            cfg.iconTheme = readIniKey(gtk3Ini, QStringLiteral("Settings"), QStringLiteral("gtk-icon-theme-name"));
        }
    }
    if (cfg.cursorTheme.isEmpty()) {
        cfg.cursorTheme = queryGsetting(QStringLiteral("cursor-theme"));
        if (cfg.cursorTheme.isEmpty()) {
            cfg.cursorTheme = readIniKey(gtk3Ini, QStringLiteral("Settings"), QStringLiteral("gtk-cursor-theme-name"));
        }
    }
}

QString ToolkitTheme::resolvedQtPlatformTheme(const ConfigData& cfg) {
    if (!cfg.qtPlatformTheme.isEmpty() && cfg.qtPlatformTheme != QStringLiteral("auto")) {
        return cfg.qtPlatformTheme;
    }
    const QStringList available = availableQtPlatformThemes();
    if (available.contains(QStringLiteral("gtk3"))) return QStringLiteral("gtk3");
    if (available.contains(QStringLiteral("qt6ct"))) return QStringLiteral("qt6ct");
    if (available.contains(QStringLiteral("xdgdesktopportal"))) return QStringLiteral("xdgdesktopportal");
    if (available.contains(QStringLiteral("kde"))) return QStringLiteral("kde");
    return {};
}

QMap<QString, QString> ToolkitTheme::environmentOverrides(const ConfigData& cfg) {
    QMap<QString, QString> env;
    env.insert(QStringLiteral("QT_QPA_PLATFORMTHEME"), resolvedQtPlatformTheme(cfg));
    env.insert(QStringLiteral("QT_STYLE_OVERRIDE"), cfg.qtStyle);
    if (!cfg.cursorTheme.isEmpty()) {
        env.insert(QStringLiteral("XCURSOR_THEME"), cfg.cursorTheme);
    }
    env.insert(QStringLiteral("XCURSOR_SIZE"), QString::number(qBound(16, cfg.cursorSize, 64)));
    return env;
}

void ToolkitTheme::applyToProcessEnvironment(const ConfigData& cfg) {
    const auto overrides = environmentOverrides(cfg);
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        const QByteArray key = it.key().toUtf8();
        if (it.value().isEmpty()) {
            ::unsetenv(key.constData());
        } else {
            ::setenv(key.constData(), it.value().toUtf8().constData(), 1);
        }
    }
}

QProcessEnvironment ToolkitTheme::childProcessEnvironment(const ConfigData& cfg) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const auto overrides = environmentOverrides(cfg);
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        if (it.value().isEmpty()) {
            env.remove(it.key());
        } else {
            env.insert(it.key(), it.value());
        }
    }
    return env;
}

void ToolkitTheme::apply(const ConfigData& cfg) {
    const QString cfgBase = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const bool preferDark = (cfg.colorScheme != QStringLiteral("prefer-light"));

    // 1. GTK 3 & GTK 4 settings.ini
    QMap<QString, QString> gtkUpdates;
    gtkUpdates.insert(QStringLiteral("gtk-application-prefer-dark-theme"),
                      preferDark ? QStringLiteral("true") : QStringLiteral("false"));
    if (!cfg.gtkTheme.isEmpty()) {
        gtkUpdates.insert(QStringLiteral("gtk-theme-name"), cfg.gtkTheme);
    }
    if (!cfg.iconTheme.isEmpty()) {
        gtkUpdates.insert(QStringLiteral("gtk-icon-theme-name"), cfg.iconTheme);
    }
    if (!cfg.cursorTheme.isEmpty()) {
        gtkUpdates.insert(QStringLiteral("gtk-cursor-theme-name"), cfg.cursorTheme);
    }
    gtkUpdates.insert(QStringLiteral("gtk-cursor-theme-size"), QString::number(qBound(16, cfg.cursorSize, 64)));

    updateIniKeys(cfgBase + QStringLiteral("/gtk-3.0/settings.ini"), QStringLiteral("Settings"), gtkUpdates);
    updateIniKeys(cfgBase + QStringLiteral("/gtk-4.0/settings.ini"), QStringLiteral("Settings"), gtkUpdates);

    // Live update via gsettings (dconf)
    setGsetting(QStringLiteral("color-scheme"),
                cfg.colorScheme.isEmpty() ? QStringLiteral("prefer-dark") : cfg.colorScheme);
    setGsetting(QStringLiteral("gtk-theme"), cfg.gtkTheme);
    setGsetting(QStringLiteral("icon-theme"), cfg.iconTheme);
    setGsetting(QStringLiteral("cursor-theme"), cfg.cursorTheme);
    setGsetting(QStringLiteral("cursor-size"), QString::number(qBound(16, cfg.cursorSize, 64)));

    // 2. Qt5 / Qt6 via qt6ct & qt5ct
    QMap<QString, QString> qtctUpdates;
    qtctUpdates.insert(QStringLiteral("style"), cfg.qtStyle.isEmpty() ? QStringLiteral("Fusion") : cfg.qtStyle);
    if (!cfg.iconTheme.isEmpty()) {
        qtctUpdates.insert(QStringLiteral("icon_theme"), cfg.iconTheme);
    }
    updateIniKeys(cfgBase + QStringLiteral("/qt6ct/qt6ct.conf"), QStringLiteral("Appearance"), qtctUpdates);
    updateIniKeys(cfgBase + QStringLiteral("/qt5ct/qt5ct.conf"), QStringLiteral("Appearance"), qtctUpdates);

    // 3. KDE/Breeze platform theme compatibility (~/.config/kdeglobals)
    if (!cfg.iconTheme.isEmpty()) {
        updateIniKeys(cfgBase + QStringLiteral("/kdeglobals"), QStringLiteral("Icons"),
                      {{QStringLiteral("Theme"), cfg.iconTheme}});
    }
    if (!cfg.qtStyle.isEmpty()) {
        updateIniKeys(cfgBase + QStringLiteral("/kdeglobals"), QStringLiteral("KDE"),
                      {{QStringLiteral("widgetStyle"), cfg.qtStyle}});
    }

    // 4. Default XCursor theme fallback
    if (!cfg.cursorTheme.isEmpty()) {
        updateIniKeys(QDir::homePath() + QStringLiteral("/.icons/default/index.theme"),
                      QStringLiteral("Icon Theme"),
                      {{QStringLiteral("Inherits"), cfg.cursorTheme}});
    }

    // 5. Process & session activation environment
    applyToProcessEnvironment(cfg);

    const auto overrides = environmentOverrides(cfg);
    QStringList setArgs;
    QStringList unsetArgs;
    for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
        if (it.value().isEmpty()) {
            unsetArgs.append(it.key());
        } else {
            setArgs.append(it.key() + QLatin1Char('=') + it.value());
        }
    }

    if (!setArgs.isEmpty()) {
        if (!QStandardPaths::findExecutable(QStringLiteral("dbus-update-activation-environment")).isEmpty()) {
            QStringList dbusArgs{QStringLiteral("--systemd")};
            dbusArgs.append(setArgs);
            QProcess::startDetached(QStringLiteral("dbus-update-activation-environment"), dbusArgs);
        }
        if (!QStandardPaths::findExecutable(QStringLiteral("systemctl")).isEmpty()) {
            QStringList sysArgs{QStringLiteral("--user"), QStringLiteral("set-environment")};
            sysArgs.append(setArgs);
            QProcess::startDetached(QStringLiteral("systemctl"), sysArgs);
        }
    }
    if (!unsetArgs.isEmpty() && !QStandardPaths::findExecutable(QStringLiteral("systemctl")).isEmpty()) {
        QStringList sysUnset{QStringLiteral("--user"), QStringLiteral("unset-environment")};
        sysUnset.append(unsetArgs);
        QProcess::startDetached(QStringLiteral("systemctl"), sysUnset);
    }
}

} // namespace lightwm
