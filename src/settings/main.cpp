#include "core/Theme.hpp"
#include "settings/SettingsBackend.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("lightwm-settings"));
    app.setOrganizationName(QStringLiteral("LightWM"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    lightwm::Theme theme;
    lightwm::settings::SettingsBackend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("Settings"), &backend);

    engine.loadFromModule(QStringLiteral("LightWM.Settings"), QStringLiteral("SettingsWindow"));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
