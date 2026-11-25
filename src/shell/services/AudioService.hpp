#pragma once

#include <QObject>

struct pa_threaded_mainloop;
struct pa_context;

namespace lightwm::shell {

// Controls default audio sink volume/mute via libpulse (PipeWire-Pulse).
class AudioService : public QObject {
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY changed)

public:
    explicit AudioService(QObject* parent = nullptr);
    ~AudioService() override;

    int volume() const { return m_volume; }
    void setVolume(int percent);

    bool muted() const { return m_muted; }
    void setMuted(bool muted);

    Q_INVOKABLE void stepVolume(int delta);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void refresh();

signals:
    void changed();
    void osdRequested(int volume, bool muted);

private:
    pa_threaded_mainloop* m_mainloop = nullptr;
    pa_context* m_ctx = nullptr;
    QString m_defaultSink;
    int m_channels = 2;
    int m_volume = 50;
    bool m_muted = false;
};

} // namespace lightwm::shell
