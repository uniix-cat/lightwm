#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

namespace lightwm {

struct ConfigData {
    // Appearance & theming
    QString themeName = QStringLiteral("pine"); // "cream" or "pine"
    int gapsInner = 8;
    int gapsOuter = 12;
    int borderWidth = 2;
    int cornerRadius = 12;
    QString fontFamily = QStringLiteral("Sans");
    QString iconFontFamily = QStringLiteral("JetBrainsMono Nerd Font Propo");
    int barHeight = 36;
    QString wallpaperPath;
    QString wallpaperMode = QStringLiteral("fill"); // "fill", "fit", "stretch", "center"

    // Workspaces & tiling
    int workspaceCount = 9;
    double dwindleSplitRatio = 0.5;

    // Input & keyboard
    QString keyboardLayouts = QStringLiteral("us,ru");
    QString keyboardOptions = QStringLiteral("grp:alt_shift_toggle");
    bool tapToClick = true;
    bool naturalScroll = true;
    double pointerSpeed = 0.0; // [-1.0, 1.0]

    // Applications
    QString terminal = QStringLiteral("foot");

    // Displays (per-output fractional scale, keyed by output name e.g. "eDP-1")
    QMap<QString, double> outputScales;
};

class ConfigStore {
public:
    static QString configFilePath();
    static ConfigData load();
    static bool save(const ConfigData& data);
};

} // namespace lightwm
