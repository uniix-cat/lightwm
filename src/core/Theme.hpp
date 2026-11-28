#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <cstdint>

namespace lightwm {

struct Rgba32 {
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t a = 0xffffffffu;
};

struct ThemePalette {
    QString id;
    QString displayName;
    bool isDark = true;

    QColor background;
    QColor surface;
    QColor surfaceElevated;
    QColor accent;
    QColor accentStrong;
    QColor textPrimary;
    QColor textSecondary;
    QColor textOnAccent;
    QColor borderInactive;
    QColor borderActive;
    QColor danger;
};

ThemePalette paletteForName(const QString& name);
Rgba32 toPremultipliedWaylandRgba(const QColor& c);

class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeId READ themeId WRITE setThemeId NOTIFY changed)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed)
    Q_PROPERTY(bool isDark READ isDark NOTIFY changed)

    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor surfaceElevated READ surfaceElevated NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor accentStrong READ accentStrong NOTIFY changed)
    Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY changed)
    Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY changed)
    Q_PROPERTY(QColor textOnAccent READ textOnAccent NOTIFY changed)
    Q_PROPERTY(QColor borderInactive READ borderInactive NOTIFY changed)
    Q_PROPERTY(QColor borderActive READ borderActive NOTIFY changed)
    Q_PROPERTY(QColor danger READ danger NOTIFY changed)

    Q_PROPERTY(int radius READ radius WRITE setRadius NOTIFY changed)
    Q_PROPERTY(int gapsInner READ gapsInner WRITE setGapsInner NOTIFY changed)
    Q_PROPERTY(int barHeight READ barHeight WRITE setBarHeight NOTIFY changed)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY changed)
    Q_PROPERTY(QString iconFontFamily READ iconFontFamily NOTIFY changed)
    Q_PROPERTY(QString wallpaperPath READ wallpaperPath NOTIFY changed)
    Q_PROPERTY(QString wallpaperMode READ wallpaperMode NOTIFY changed)
    Q_PROPERTY(bool blurEnabled READ blurEnabled NOTIFY changed)
    Q_PROPERTY(int blurStrength READ blurStrength NOTIFY changed)
    Q_PROPERTY(double glassOpacity READ glassOpacity NOTIFY changed)

public:
    explicit Theme(QObject* parent = nullptr);

    QString themeId() const { return m_palette.id; }
    void setThemeId(const QString& id);

    QString displayName() const { return m_palette.displayName; }
    bool isDark() const { return m_palette.isDark; }

    QColor background() const { return m_palette.background; }
    QColor surface() const { return m_palette.surface; }
    QColor surfaceElevated() const { return m_palette.surfaceElevated; }
    QColor accent() const { return m_palette.accent; }
    QColor accentStrong() const { return m_palette.accentStrong; }
    QColor textPrimary() const { return m_palette.textPrimary; }
    QColor textSecondary() const { return m_palette.textSecondary; }
    QColor textOnAccent() const { return m_palette.textOnAccent; }
    QColor borderInactive() const { return m_palette.borderInactive; }
    QColor borderActive() const { return m_palette.borderActive; }
    QColor danger() const { return m_palette.danger; }

    int radius() const { return m_radius; }
    void setRadius(int radius);

    int gapsInner() const { return m_gapsInner; }
    void setGapsInner(int gaps);

    int barHeight() const { return m_barHeight; }
    void setBarHeight(int height);

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& family);

    QString iconFontFamily() const { return m_iconFontFamily; }

    QString wallpaperPath() const { return m_wallpaperPath; }
    QString wallpaperMode() const { return m_wallpaperMode; }
    bool blurEnabled() const { return m_blurEnabled; }
    int blurStrength() const { return m_blurStrength; }
    double glassOpacity() const { return m_glassOpacity; }

    Q_INVOKABLE void reloadFromConfig();

signals:
    void changed();

private:
    QFileSystemWatcher m_watcher;
    ThemePalette m_palette;
    int m_radius = 12;
    int m_gapsInner = 8;
    int m_barHeight = 36;
    QString m_fontFamily = QStringLiteral("Sans");
    QString m_iconFontFamily = QStringLiteral("JetBrainsMono Nerd Font Propo");
    QString m_wallpaperPath;
    QString m_wallpaperMode = QStringLiteral("fill");
    bool m_blurEnabled = true;
    int m_blurStrength = 48;
    double m_glassOpacity = 0.72;
};

} // namespace lightwm
