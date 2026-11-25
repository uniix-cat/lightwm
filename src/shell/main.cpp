#include "core/Theme.hpp"
#include "shell/WmClient.hpp"
#include "shell/services/AppLauncherService.hpp"
#include "shell/services/AudioService.hpp"
#include "shell/services/BatteryService.hpp"
#include "shell/services/BluetoothService.hpp"
#include "shell/services/BrightnessService.hpp"
#include "shell/services/LockscreenService.hpp"
#include "shell/services/NetworkService.hpp"
#include "shell/services/NotificationService.hpp"
#include "shell/services/ScreenshotService.hpp"
#include "shell/services/TrayService.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("lightwm-shell"));
    app.setOrganizationName(QStringLiteral("LightWM"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    lightwm::Theme theme;
    lightwm::shell::WmClient wmClient;
    lightwm::shell::AppLauncherService launcherService;
    lightwm::shell::NetworkService networkService;
    lightwm::shell::BluetoothService bluetoothService;
    lightwm::shell::AudioService audioService;
    lightwm::shell::BatteryService batteryService;
    lightwm::shell::BrightnessService brightnessService;
    lightwm::shell::NotificationService notificationService;
    lightwm::shell::TrayService trayService;
    lightwm::shell::ScreenshotService screenshotService;
    lightwm::shell::LockscreenService lockscreenService;

    QObject::connect(&wmClient, &lightwm::shell::WmClient::audioStepRequested,
                     &audioService, &lightwm::shell::AudioService::stepVolume);
    QObject::connect(&wmClient, &lightwm::shell::WmClient::audioMuteToggleRequested,
                     &audioService, &lightwm::shell::AudioService::toggleMute);
    QObject::connect(&wmClient, &lightwm::shell::WmClient::brightnessStepRequested,
                     &brightnessService, &lightwm::shell::BrightnessService::stepBrightness);
    QObject::connect(&wmClient, &lightwm::shell::WmClient::lockScreenRequested,
                     &lockscreenService, &lightwm::shell::LockscreenService::lock);

    QQmlApplicationEngine engine;
    lockscreenService.setEngine(&engine);

    engine.rootContext()->setContextProperty(QStringLiteral("Theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("Wm"), &wmClient);
    engine.rootContext()->setContextProperty(QStringLiteral("Launcher"), &launcherService);
    engine.rootContext()->setContextProperty(QStringLiteral("Network"), &networkService);
    engine.rootContext()->setContextProperty(QStringLiteral("Bluetooth"), &bluetoothService);
    engine.rootContext()->setContextProperty(QStringLiteral("Audio"), &audioService);
    engine.rootContext()->setContextProperty(QStringLiteral("Battery"), &batteryService);
    engine.rootContext()->setContextProperty(QStringLiteral("Brightness"), &brightnessService);
    engine.rootContext()->setContextProperty(QStringLiteral("Notifications"), &notificationService);
    engine.rootContext()->setContextProperty(QStringLiteral("Tray"), &trayService);
    engine.rootContext()->setContextProperty(QStringLiteral("Screenshot"), &screenshotService);
    engine.rootContext()->setContextProperty(QStringLiteral("Lockscreen"), &lockscreenService);

    engine.loadFromModule(QStringLiteral("LightWM.Shell"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
