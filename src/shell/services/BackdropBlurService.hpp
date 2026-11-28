#pragma once

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QQuickImageProvider>
#include <QSet>
#include <QString>
#include <memory>

class QScreen;
struct wl_display;
struct wl_registry;
struct wl_shm;
struct zwlr_screencopy_manager_v1;

namespace lightwm::shell {

struct BackdropFrameStore {
    QMutex mutex;
    QHash<QString, QImage> frames;
};

class BackdropImageProvider : public QQuickImageProvider {
public:
    explicit BackdropImageProvider(std::shared_ptr<BackdropFrameStore> store);
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    std::shared_ptr<BackdropFrameStore> m_store;
};

class BackdropBlurService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY backdropUpdated)

public:
    explicit BackdropBlurService(QObject* parent = nullptr);
    ~BackdropBlurService() override;

    bool available() const { return m_screencopy != nullptr && m_shm != nullptr; }
    int revision() const { return m_revision; }
    uint32_t screencopyVersion() const { return m_screencopyVersion; }
    QQuickImageProvider* createImageProvider() const;

    Q_INVOKABLE void captureScreen(QScreen* screen);
    Q_INVOKABLE void capturePrimary();
    Q_INVOKABLE bool hasFrame(const QString& screenName) const;

    // Protocol callbacks
    void onRegistryGlobal(uint32_t name, const char* iface, uint32_t version);

signals:
    void backdropUpdated();

private:
    friend struct ScreencopyCapture;

    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    wl_shm* m_shm = nullptr;
    zwlr_screencopy_manager_v1* m_screencopy = nullptr;
    uint32_t m_screencopyVersion = 0;

    std::shared_ptr<BackdropFrameStore> m_store;
    QSet<QString> m_inFlightScreens;
    int m_revision = 0;
};

} // namespace lightwm::shell
