#include "core/Theme.hpp"
#include "core/Config.hpp"
#include <QFontDatabase>

namespace lightwm {

ThemePalette paletteForName(const QString& name) {
    if (name == QStringLiteral("cream")) {
        // Light theme: Cream #FFFDD0 (255, 253, 208) + Soft Yellow #FFEC8B (255, 236, 139)
        return ThemePalette{
            .id = QStringLiteral("cream"),
            .displayName = QStringLiteral("Cream & Butter (Light)"),
            .isDark = false,
            .background = QColor(255, 253, 208),
            .surface = QColor(248, 242, 186),
            .surfaceElevated = QColor(255, 254, 232),
            .accent = QColor(255, 236, 139),
            .accentStrong = QColor(224, 194, 68),
            .textPrimary = QColor(42, 38, 24),
            .textSecondary = QColor(105, 96, 62),
            .textOnAccent = QColor(36, 32, 18),
            .borderInactive = QColor(216, 205, 148),
            .borderActive = QColor(219, 182, 54),
            .danger = QColor(198, 68, 68),
        };
    }

    // Default dark theme: Slate Gray + Pine Green
    return ThemePalette{
        .id = QStringLiteral("pine"),
        .displayName = QStringLiteral("Slate & Pine (Dark)"),
        .isDark = true,
        .background = QColor(30, 34, 36),
        .surface = QColor(40, 45, 48),
        .surfaceElevated = QColor(51, 58, 62),
        .accent = QColor(45, 106, 79),
        .accentStrong = QColor(64, 145, 108),
        .textPrimary = QColor(236, 239, 241),
        .textSecondary = QColor(155, 165, 171),
        .textOnAccent = QColor(244, 248, 246),
        .borderInactive = QColor(60, 68, 72),
        .borderActive = QColor(64, 145, 108),
        .danger = QColor(217, 83, 79),
    };
}

Rgba32 toPremultipliedWaylandRgba(const QColor& c) {
    auto scale = [](double v) -> uint32_t {
        const double clamped = qBound(0.0, v, 1.0);
        return static_cast<uint32_t>(clamped * 4294967295.0 + 0.5);
    };
    const double a = c.alphaF();
    return Rgba32{
        .r = scale(c.redF() * a),
        .g = scale(c.greenF() * a),
        .b = scale(c.blueF() * a),
        .a = scale(a),
    };
}

Theme::Theme(QObject* parent) : QObject(parent) {
    reloadFromConfig();
    const QString cfgPath = ConfigStore::configFilePath();
    m_watcher.addPath(cfgPath);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this, cfgPath]() {
        if (!m_watcher.files().contains(cfgPath)) {
            m_watcher.addPath(cfgPath);
        }
        reloadFromConfig();
    });
}

void Theme::setThemeId(const QString& id) {
    const ThemePalette next = paletteForName(id);
    if (m_palette.id == next.id) {
        return;
    }
    m_palette = next;
    emit changed();
}

void Theme::setRadius(int radius) {
    if (m_radius == radius) return;
    m_radius = radius;
    emit changed();
}

void Theme::setGapsInner(int gaps) {
    if (m_gapsInner == gaps) return;
    m_gapsInner = gaps;
    emit changed();
}

void Theme::setBarHeight(int height) {
    if (m_barHeight == height) return;
    m_barHeight = height;
    emit changed();
}

void Theme::setFontFamily(const QString& family) {
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    emit changed();
}

void Theme::reloadFromConfig() {
    const ConfigData cfg = ConfigStore::load();
    m_palette = paletteForName(cfg.themeName);
    m_radius = cfg.cornerRadius;
    m_gapsInner = cfg.gapsInner;
    m_barHeight = cfg.barHeight;
    m_fontFamily = cfg.fontFamily;
    const QStringList families = QFontDatabase::families();
    if (families.contains(cfg.iconFontFamily)) {
        m_iconFontFamily = cfg.iconFontFamily;
    } else {
        m_iconFontFamily = cfg.iconFontFamily;
        for (const QString& f : families) {
            if (f.contains(QStringLiteral("Nerd Font"), Qt::CaseInsensitive)) {
                m_iconFontFamily = f;
                break;
            }
        }
    }
    m_wallpaperPath = cfg.wallpaperPath;
    m_wallpaperMode = cfg.wallpaperMode;
    m_blurEnabled = cfg.blurEnabled;
    m_blurStrength = cfg.blurStrength;
    m_glassOpacity = cfg.glassOpacity;
    emit changed();
}

} // namespace lightwm
