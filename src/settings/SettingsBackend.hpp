#pragma once

#include "core/Config.hpp"
#include <QObject>
#include <QUrl>
#include <QVariantList>

namespace lightwm::settings {

class SettingsBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY changed)
    Q_PROPERTY(int gapsInner READ gapsInner WRITE setGapsInner NOTIFY changed)
    Q_PROPERTY(int gapsOuter READ gapsOuter WRITE setGapsOuter NOTIFY changed)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth NOTIFY changed)
    Q_PROPERTY(int cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY changed)
    Q_PROPERTY(int workspaceCount READ workspaceCount WRITE setWorkspaceCount NOTIFY changed)
    Q_PROPERTY(QString keyboardLayouts READ keyboardLayouts WRITE setKeyboardLayouts NOTIFY changed)
    Q_PROPERTY(QString keyboardOptions READ keyboardOptions WRITE setKeyboardOptions NOTIFY changed)
    Q_PROPERTY(bool tapToClick READ tapToClick WRITE setTapToClick NOTIFY changed)
    Q_PROPERTY(bool naturalScroll READ naturalScroll WRITE setNaturalScroll NOTIFY changed)
    Q_PROPERTY(double pointerSpeed READ pointerSpeed WRITE setPointerSpeed NOTIFY changed)
    Q_PROPERTY(QString terminal READ terminal WRITE setTerminal NOTIFY changed)
    Q_PROPERTY(QString wallpaperPath READ wallpaperPath WRITE setWallpaperPath NOTIFY changed)
    Q_PROPERTY(QString wallpaperMode READ wallpaperMode WRITE setWallpaperMode NOTIFY changed)
    Q_PROPERTY(QVariantList displays READ displays CONSTANT)

public:
    explicit SettingsBackend(QObject* parent = nullptr);

    QString themeName() const { return m_cfg.themeName; }
    void setThemeName(const QString& v) { m_cfg.themeName = v; applyAndSave(); }

    int gapsInner() const { return m_cfg.gapsInner; }
    void setGapsInner(int v) { m_cfg.gapsInner = v; applyAndSave(); }

    int gapsOuter() const { return m_cfg.gapsOuter; }
    void setGapsOuter(int v) { m_cfg.gapsOuter = v; applyAndSave(); }

    int borderWidth() const { return m_cfg.borderWidth; }
    void setBorderWidth(int v) { m_cfg.borderWidth = v; applyAndSave(); }

    int cornerRadius() const { return m_cfg.cornerRadius; }
    void setCornerRadius(int v) { m_cfg.cornerRadius = v; applyAndSave(); }

    int workspaceCount() const { return m_cfg.workspaceCount; }
    void setWorkspaceCount(int v) { m_cfg.workspaceCount = v; applyAndSave(); }

    QString keyboardLayouts() const { return m_cfg.keyboardLayouts; }
    void setKeyboardLayouts(const QString& v) { m_cfg.keyboardLayouts = v; applyAndSave(); }

    QString keyboardOptions() const { return m_cfg.keyboardOptions; }
    void setKeyboardOptions(const QString& v) { m_cfg.keyboardOptions = v; applyAndSave(); }

    bool tapToClick() const { return m_cfg.tapToClick; }
    void setTapToClick(bool v) { m_cfg.tapToClick = v; applyAndSave(); }

    bool naturalScroll() const { return m_cfg.naturalScroll; }
    void setNaturalScroll(bool v) { m_cfg.naturalScroll = v; applyAndSave(); }

    double pointerSpeed() const { return m_cfg.pointerSpeed; }
    void setPointerSpeed(double v) { m_cfg.pointerSpeed = qBound(-1.0, v, 1.0); applyAndSave(); }

    QString terminal() const { return m_cfg.terminal; }
    void setTerminal(const QString& v) { m_cfg.terminal = v; applyAndSave(); }

    QString wallpaperPath() const { return m_cfg.wallpaperPath; }
    void setWallpaperPath(const QString& v) { m_cfg.wallpaperPath = v; applyAndSave(); }

    QString wallpaperMode() const { return m_cfg.wallpaperMode; }
    void setWallpaperMode(const QString& v) { m_cfg.wallpaperMode = v; applyAndSave(); }

    Q_INVOKABLE void setWallpaperFromUrl(const QUrl& url);

    QVariantList displays() const;

signals:
    void changed();

private:
    void applyAndSave();
    void notifyWmReload();

    ConfigData m_cfg;
};

} // namespace lightwm::settings
