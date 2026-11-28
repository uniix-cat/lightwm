#include "shell/services/AppLauncherService.hpp"
#include "core/Config.hpp"
#include "core/ToolkitTheme.hpp"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QVariantMap>

namespace lightwm::shell {

AppLauncherService::AppLauncherService(QObject* parent) : QObject(parent) {
    refresh();
}

void AppLauncherService::refresh() {
    m_apps.clear();
    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QSet<QString> seenNames;

    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        const QFileInfoList entries = dir.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QFileInfo& fi : entries) {
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            bool inDesktopEntry = false;
            bool noDisplay = false;
            QString name;
            QString exec;
            QString icon;
            QString comment;

            QTextStream in(&f);
            while (!in.atEnd()) {
                const QString line = in.readLine().trimmed();
                if (line.startsWith('[')) {
                    inDesktopEntry = (line == QStringLiteral("[Desktop Entry]"));
                    continue;
                }
                if (!inDesktopEntry || line.isEmpty() || line.startsWith('#')) continue;

                const int eq = line.indexOf('=');
                if (eq <= 0) continue;
                const QString key = line.left(eq).trimmed();
                const QString val = line.mid(eq + 1).trimmed();

                if (key == QStringLiteral("NoDisplay") || key == QStringLiteral("Hidden")) {
                    if (val.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
                        noDisplay = true;
                        break;
                    }
                } else if (key == QStringLiteral("Name") && name.isEmpty()) {
                    name = val;
                } else if (key == QStringLiteral("Exec") && exec.isEmpty()) {
                    exec = val;
                } else if (key == QStringLiteral("Icon") && icon.isEmpty()) {
                    icon = val;
                } else if (key == QStringLiteral("Comment") && comment.isEmpty()) {
                    comment = val;
                }
            }

            if (noDisplay || name.isEmpty() || exec.isEmpty() || seenNames.contains(name)) {
                continue;
            }
            seenNames.insert(name);

            exec.remove(QRegularExpression(QStringLiteral("%[fFuUdDnNickvm]")));
            exec = exec.trimmed();

            QVariantMap item;
            item.insert(QStringLiteral("name"), name);
            item.insert(QStringLiteral("exec"), exec);
            item.insert(QStringLiteral("icon"), icon);
            item.insert(QStringLiteral("comment"), comment);
            m_apps.append(item);
        }
    }

    std::sort(m_apps.begin(), m_apps.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value(QStringLiteral("name")).toString().compare(
                   b.toMap().value(QStringLiteral("name")).toString(), Qt::CaseInsensitive) < 0;
    });
    emit appsChanged();
}

QVariantList AppLauncherService::search(const QString& query) const {
    if (query.trimmed().isEmpty()) {
        return m_apps;
    }
    const QString q = query.trimmed();
    QVariantList result;
    for (const QVariant& v : m_apps) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("name")).toString().contains(q, Qt::CaseInsensitive) ||
            m.value(QStringLiteral("comment")).toString().contains(q, Qt::CaseInsensitive)) {
            result.append(m);
        }
    }
    return result;
}

void AppLauncherService::launch(const QString& execLine) {
    if (execLine.isEmpty()) return;
    QStringList parts = QProcess::splitCommand(execLine);
    if (parts.isEmpty()) return;
    const QString program = parts.takeFirst();
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(parts);
    proc.setProcessEnvironment(ToolkitTheme::childProcessEnvironment(ConfigStore::load()));
    proc.startDetached();
}

} // namespace lightwm::shell
