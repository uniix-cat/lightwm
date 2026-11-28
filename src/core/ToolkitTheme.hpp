#pragma once

#include "core/Config.hpp"
#include <QMap>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace lightwm {

class ToolkitTheme {
public:
    static QStringList availableGtkThemes();
    static QStringList availableIconThemes();
    static QStringList availableCursorThemes();
    static QStringList availableQtStyles();
    static QStringList availableQtPlatformThemes();

    // Fills empty GTK/icon/cursor fields from active system settings (read-only)
    static void populateSystemDefaults(ConfigData& cfg);

    static QString resolvedQtPlatformTheme(const ConfigData& cfg);

    // Maps env var name -> value (empty string means unset)
    static QMap<QString, QString> environmentOverrides(const ConfigData& cfg);
    static void applyToProcessEnvironment(const ConfigData& cfg);
    static QProcessEnvironment childProcessEnvironment(const ConfigData& cfg);

    // Writes GTK3/GTK4/qt6ct/qt5ct/kdeglobals/cursor files, updates gsettings & session env
    static void apply(const ConfigData& cfg);
};

} // namespace lightwm
