#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace lightwm {

struct ShortcutEntry {
    QString action;  // e.g. "spawn_terminal", "close_focused", ..., "custom"
    QString mods;    // e.g. "Super", "Super+Shift", "Ctrl+Alt", ""
    QString key;     // XKB keysym name, e.g. "q", "Return", "Print"
    QString command; // shell command when action == "custom"
};

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
    bool blurEnabled = true;
    int blurStrength = 48;     // 0..64 (MultiEffect.blurMax)
    double glassOpacity = 0.72; // 0.35..0.95

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

    // Shortcuts
    QList<ShortcutEntry> shortcuts;

    static QList<ShortcutEntry> defaultShortcuts();

    // Displays (per-output fractional scale, keyed by output name e.g. "eDP-1")
    QMap<QString, double> outputScales;

    // Qt & GTK toolkit theming
    QString colorScheme = QStringLiteral("prefer-dark"); // "prefer-dark", "prefer-light", "default"
    QString gtkTheme;
    QString iconTheme;
    QString cursorTheme;
    int cursorSize = 24;
    QString qtPlatformTheme = QStringLiteral("auto"); // "auto", "gtk3", "qt6ct", "kde", "xdgdesktopportal"
    QString qtStyle;                                  // empty = platform default, or "Fusion", "Breeze", etc.
};

class ConfigStore {
public:
    static QString configFilePath();
    static ConfigData load();
    static bool save(const ConfigData& data);
};

} // namespace lightwm
