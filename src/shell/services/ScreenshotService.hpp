#pragma once

#include <QObject>
#include <QString>

namespace lightwm::shell {

// Hybrid screenshot backend: QML selection overlay + grim + wl-copy.
class ScreenshotService : public QObject {
    Q_OBJECT

public:
    explicit ScreenshotService(QObject* parent = nullptr);

    Q_INVOKABLE void captureFullScreen();
    Q_INVOKABLE void captureRegion(int x, int y, int width, int height);

signals:
    void screenshotSaved(const QString& filePath);
    void screenshotFailed(const QString& error);

private:
    QString nextOutputPath() const;
    void copyFileToClipboard(const QString& filePath);
};

} // namespace lightwm::shell
