#pragma once

#include <QObject>
#include <QVariantList>

namespace lightwm::shell {

class AppLauncherService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)

public:
    explicit AppLauncherService(QObject* parent = nullptr);

    QVariantList apps() const { return m_apps; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantList search(const QString& query) const;
    Q_INVOKABLE void launch(const QString& execLine);

signals:
    void appsChanged();

private:
    QVariantList m_apps;
};

} // namespace lightwm::shell
