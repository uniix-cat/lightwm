#include "shell/services/ScreenshotService.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

namespace lightwm::shell {

ScreenshotService::ScreenshotService(QObject* parent) : QObject(parent) {}

QString ScreenshotService::nextOutputPath() const {
    QString pics = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pics.isEmpty()) pics = QDir::homePath() + QStringLiteral("/Pictures");
    const QString dirPath = pics + QStringLiteral("/Screenshots");
    QDir().mkpath(dirPath);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return dirPath + QStringLiteral("/lightwm-") + stamp + QStringLiteral(".png");
}

void ScreenshotService::captureFullScreen() {
    const QString outPath = nextOutputPath();
    auto* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc, outPath](int code, QProcess::ExitStatus) {
        proc->deleteLater();
        if (code == 0 && QFile::exists(outPath)) {
            copyFileToClipboard(outPath);
            emit screenshotSaved(outPath);
        } else {
            emit screenshotFailed(QStringLiteral("grim failed (is grim installed?)"));
        }
    });
    proc->start(QStringLiteral("grim"), {outPath});
}

void ScreenshotService::captureRegion(int x, int y, int width, int height) {
    if (width <= 1 || height <= 1) {
        emit screenshotFailed(QStringLiteral("Selection too small"));
        return;
    }
    const QString outPath = nextOutputPath();
    const QString geom = QStringLiteral("%1,%2 %3x%4").arg(x).arg(y).arg(width).arg(height);

    auto* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc, outPath](int code, QProcess::ExitStatus) {
        proc->deleteLater();
        if (code == 0 && QFile::exists(outPath)) {
            copyFileToClipboard(outPath);
            emit screenshotSaved(outPath);
        } else {
            emit screenshotFailed(QStringLiteral("grim region capture failed"));
        }
    });
    proc->start(QStringLiteral("grim"), {QStringLiteral("-g"), geom, outPath});
}

void ScreenshotService::copyFileToClipboard(const QString& filePath) {
    QProcess* cp = new QProcess(this);
    cp->setStandardInputFile(filePath);
    connect(cp, &QProcess::finished, cp, &QObject::deleteLater);
    cp->start(QStringLiteral("wl-copy"), {QStringLiteral("--type"), QStringLiteral("image/png")});
}

} // namespace lightwm::shell
