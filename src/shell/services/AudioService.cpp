#include "shell/services/AudioService.hpp"

#include <QMetaObject>
#include <QtGlobal>
#include <pulse/pulseaudio.h>

namespace lightwm::shell {

AudioService::AudioService(QObject* parent) : QObject(parent) {
    m_mainloop = pa_threaded_mainloop_new();
    if (!m_mainloop) return;

    pa_mainloop_api* api = pa_threaded_mainloop_get_api(m_mainloop);
    m_ctx = pa_context_new(api, "lightwm-shell");
    if (!m_ctx) return;

    pa_context_set_state_callback(m_ctx, [](pa_context* c, void* userdata) {
        auto* self = static_cast<AudioService*>(userdata);
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            QMetaObject::invokeMethod(self, &AudioService::refresh, Qt::QueuedConnection);
        }
    }, this);

    pa_context_connect(m_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_threaded_mainloop_start(m_mainloop);
}

AudioService::~AudioService() {
    if (m_mainloop) pa_threaded_mainloop_stop(m_mainloop);
    if (m_ctx) {
        pa_context_disconnect(m_ctx);
        pa_context_unref(m_ctx);
    }
    if (m_mainloop) pa_threaded_mainloop_free(m_mainloop);
}

void AudioService::refresh() {
    if (!m_ctx || !m_mainloop) return;
    pa_threaded_mainloop_lock(m_mainloop);
    if (pa_context_get_state(m_ctx) == PA_CONTEXT_READY) {
        pa_operation* op = pa_context_get_server_info(m_ctx, [](pa_context* c, const pa_server_info* info, void* userdata) {
            auto* self = static_cast<AudioService*>(userdata);
            if (!info || !info->default_sink_name) return;
            const QString sink = QString::fromUtf8(info->default_sink_name);
            pa_operation* sinkOp = pa_context_get_sink_info_by_name(c, info->default_sink_name,
                [](pa_context*, const pa_sink_info* sinfo, int eol, void* ud) {
                    if (eol || !sinfo) return;
                    auto* me = static_cast<AudioService*>(ud);
                    const pa_volume_t avg = pa_cvolume_avg(&sinfo->volume);
                    const int pct = qRound((static_cast<double>(avg) * 100.0) / PA_VOLUME_NORM);
                    const bool mut = (sinfo->mute != 0);
                    const int ch = sinfo->volume.channels;
                    const QString sinkName = QString::fromUtf8(sinfo->name);
                    QMetaObject::invokeMethod(me, [me, pct, mut, ch, sinkName]() {
                        me->m_defaultSink = sinkName;
                        me->m_channels = ch;
                        me->m_volume = qBound(0, pct, 150);
                        me->m_muted = mut;
                        emit me->changed();
                    }, Qt::QueuedConnection);
                }, self);
            if (sinkOp) pa_operation_unref(sinkOp);
            (void)sink;
        }, this);
        if (op) pa_operation_unref(op);
    }
    pa_threaded_mainloop_unlock(m_mainloop);
}

void AudioService::setVolume(int percent) {
    const int clamped = qBound(0, percent, 150);
    m_volume = clamped;
    emit changed();

    if (!m_ctx || !m_mainloop || m_defaultSink.isEmpty()) return;
    pa_cvolume cv;
    pa_cvolume_set(&cv, static_cast<unsigned>(qMax(1, m_channels)),
                  static_cast<pa_volume_t>((static_cast<double>(clamped) / 100.0) * PA_VOLUME_NORM));

    pa_threaded_mainloop_lock(m_mainloop);
    if (pa_context_get_state(m_ctx) == PA_CONTEXT_READY) {
        pa_operation* op = pa_context_set_sink_volume_by_name(
            m_ctx, m_defaultSink.toUtf8().constData(), &cv, nullptr, nullptr);
        if (op) pa_operation_unref(op);
    }
    pa_threaded_mainloop_unlock(m_mainloop);
}

void AudioService::setMuted(bool muted) {
    m_muted = muted;
    emit changed();

    if (!m_ctx || !m_mainloop || m_defaultSink.isEmpty()) return;
    pa_threaded_mainloop_lock(m_mainloop);
    if (pa_context_get_state(m_ctx) == PA_CONTEXT_READY) {
        pa_operation* op = pa_context_set_sink_mute_by_name(
            m_ctx, m_defaultSink.toUtf8().constData(), muted ? 1 : 0, nullptr, nullptr);
        if (op) pa_operation_unref(op);
    }
    pa_threaded_mainloop_unlock(m_mainloop);
}

void AudioService::stepVolume(int delta) {
    setVolume(m_volume + delta);
    emit osdRequested(m_volume, m_muted);
}

void AudioService::toggleMute() {
    setMuted(!m_muted);
    emit osdRequested(m_volume, m_muted);
}

} // namespace lightwm::shell
