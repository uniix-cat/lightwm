#include "wm/OutputScaleManager.hpp"

#include "wlr-output-management-unstable-v1-client-protocol.h"

#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <wayland-client.h>

namespace lightwm::wm {

namespace {

void modeSize(void* data, zwlr_output_mode_v1*, int32_t w, int32_t h) {
    auto* m = static_cast<OutputScaleManager::ModeInfo*>(data);
    m->width = w;
    m->height = h;
}

void modeRefresh(void* data, zwlr_output_mode_v1*, int32_t refresh) {
    static_cast<OutputScaleManager::ModeInfo*>(data)->refreshMhz = refresh;
}

void modePreferred(void* data, zwlr_output_mode_v1*) {
    static_cast<OutputScaleManager::ModeInfo*>(data)->preferred = true;
}

void modeFinished(void* data, zwlr_output_mode_v1*) {
    auto* m = static_cast<OutputScaleManager::ModeInfo*>(data);
    m->head->mgr->onModeFinished(m);
}

const zwlr_output_mode_v1_listener kModeListener = {
    .size = modeSize,
    .refresh = modeRefresh,
    .preferred = modePreferred,
    .finished = modeFinished,
};

void headName(void* data, zwlr_output_head_v1*, const char* name) {
    static_cast<OutputScaleManager::HeadInfo*>(data)->name = name ? name : "";
}

void headDescription(void* data, zwlr_output_head_v1*, const char* desc) {
    static_cast<OutputScaleManager::HeadInfo*>(data)->description = desc ? desc : "";
}

void headPhysicalSize(void*, zwlr_output_head_v1*, int32_t, int32_t) {}

void headMode(void* data, zwlr_output_head_v1*, zwlr_output_mode_v1* wlMode) {
    auto* head = static_cast<OutputScaleManager::HeadInfo*>(data);
    auto mode = std::make_unique<OutputScaleManager::ModeInfo>();
    mode->wl = wlMode;
    mode->head = head;
    zwlr_output_mode_v1_add_listener(wlMode, &kModeListener, mode.get());
    head->modes.push_back(std::move(mode));
}

void headEnabled(void* data, zwlr_output_head_v1*, int32_t enabled) {
    static_cast<OutputScaleManager::HeadInfo*>(data)->enabled = (enabled != 0);
}

void headCurrentMode(void* data, zwlr_output_head_v1*, zwlr_output_mode_v1* wlMode) {
    static_cast<OutputScaleManager::HeadInfo*>(data)->currentMode = wlMode;
}

void headPosition(void* data, zwlr_output_head_v1*, int32_t x, int32_t y) {
    auto* head = static_cast<OutputScaleManager::HeadInfo*>(data);
    head->x = x;
    head->y = y;
}

void headTransform(void*, zwlr_output_head_v1*, int32_t) {}

void headScale(void* data, zwlr_output_head_v1*, wl_fixed_t scale) {
    static_cast<OutputScaleManager::HeadInfo*>(data)->scale = wl_fixed_to_double(scale);
}

void headFinished(void* data, zwlr_output_head_v1*) {
    auto* head = static_cast<OutputScaleManager::HeadInfo*>(data);
    head->mgr->onHeadFinished(head);
}

void headMake(void*, zwlr_output_head_v1*, const char*) {}
void headModel(void*, zwlr_output_head_v1*, const char*) {}
void headSerialNumber(void*, zwlr_output_head_v1*, const char*) {}
void headAdaptiveSync(void*, zwlr_output_head_v1*, uint32_t) {}

const zwlr_output_head_v1_listener kHeadListener = {
    .name = headName,
    .description = headDescription,
    .physical_size = headPhysicalSize,
    .mode = headMode,
    .enabled = headEnabled,
    .current_mode = headCurrentMode,
    .position = headPosition,
    .transform = headTransform,
    .scale = headScale,
    .finished = headFinished,
    .make = headMake,
    .model = headModel,
    .serial_number = headSerialNumber,
    .adaptive_sync = headAdaptiveSync,
};

void mgrHead(void* data, zwlr_output_manager_v1*, zwlr_output_head_v1* head) {
    static_cast<OutputScaleManager*>(data)->onNewHead(head);
}

void mgrDone(void* data, zwlr_output_manager_v1*, uint32_t serial) {
    static_cast<OutputScaleManager*>(data)->onManagerDone(serial);
}

void mgrFinished(void* data, zwlr_output_manager_v1*) {
    static_cast<OutputScaleManager*>(data)->onManagerFinished();
}

const zwlr_output_manager_v1_listener kManagerListener = {
    .head = mgrHead,
    .done = mgrDone,
    .finished = mgrFinished,
};

void cfgSucceeded(void* data, zwlr_output_configuration_v1* cfg) {
    static_cast<OutputScaleManager*>(data)->onConfigSucceeded(cfg);
}

void cfgFailed(void* data, zwlr_output_configuration_v1* cfg) {
    static_cast<OutputScaleManager*>(data)->onConfigFailed(cfg);
}

void cfgCancelled(void* data, zwlr_output_configuration_v1* cfg) {
    static_cast<OutputScaleManager*>(data)->onConfigCancelled(cfg);
}

const zwlr_output_configuration_v1_listener kConfigListener = {
    .succeeded = cfgSucceeded,
    .failed = cfgFailed,
    .cancelled = cfgCancelled,
};

} // namespace

OutputScaleManager::OutputScaleManager(StateChangedCallback onStateChanged)
    : m_onStateChanged(std::move(onStateChanged)) {}

OutputScaleManager::~OutputScaleManager() {
    if (m_inFlightConfig) {
        zwlr_output_configuration_v1_destroy(m_inFlightConfig);
        m_inFlightConfig = nullptr;
    }
    for (auto& head : m_heads) {
        destroyHeadResources(head.get());
    }
    m_heads.clear();
    if (m_mgr) {
        zwlr_output_manager_v1_destroy(m_mgr);
        m_mgr = nullptr;
    }
}

void OutputScaleManager::bind(wl_registry* registry, uint32_t name, uint32_t version) {
    m_version = std::min(version, 4u);
    m_mgr = static_cast<zwlr_output_manager_v1*>(
        wl_registry_bind(registry, name, &zwlr_output_manager_v1_interface, m_version));
    zwlr_output_manager_v1_add_listener(m_mgr, &kManagerListener, this);
}

void OutputScaleManager::setDesiredScales(const QMap<QString, double>& scales) {
    m_desiredScales.clear();
    for (auto it = scales.constBegin(); it != scales.constEnd(); ++it) {
        if (!it.key().isEmpty() && it.value() > 0.0) {
            m_desiredScales[it.key().toStdString()] = std::clamp(it.value(), 0.5, 4.0);
        }
    }
    m_failedPending = false;
    applyIfNeeded();
}

QJsonArray OutputScaleManager::outputsJson() const {
    QJsonArray arr;
    for (const auto& head : m_heads) {
        if (head->finished || head->name.empty()) continue;

        QJsonObject obj;
        obj.insert(QStringLiteral("name"), QString::fromStdString(head->name));
        obj.insert(QStringLiteral("description"), QString::fromStdString(head->description));
        obj.insert(QStringLiteral("enabled"), head->enabled);
        obj.insert(QStringLiteral("width"), head->width);
        obj.insert(QStringLiteral("height"), head->height);
        obj.insert(QStringLiteral("refresh"), static_cast<int>(std::lround(head->refreshMhz / 1000.0)));
        const double s = head->scale > 0.0 ? head->scale : 1.0;
        obj.insert(QStringLiteral("scale"), s);
        obj.insert(QStringLiteral("logicalWidth"), static_cast<int>(std::lround(head->width / s)));
        obj.insert(QStringLiteral("logicalHeight"), static_cast<int>(std::lround(head->height / s)));
        arr.append(obj);
    }
    return arr;
}

void OutputScaleManager::onNewHead(zwlr_output_head_v1* wlHead) {
    auto head = std::make_unique<HeadInfo>();
    head->mgr = this;
    head->wl = wlHead;
    zwlr_output_head_v1_add_listener(wlHead, &kHeadListener, head.get());
    m_heads.push_back(std::move(head));
}

void OutputScaleManager::destroyHeadResources(HeadInfo* head) {
    if (!head) return;
    for (auto& m : head->modes) {
        if (m->wl) {
            if (m_version >= 3) {
                zwlr_output_mode_v1_release(m->wl);
            } else {
                zwlr_output_mode_v1_destroy(m->wl);
            }
            m->wl = nullptr;
        }
    }
    head->modes.clear();
    if (head->wl) {
        if (m_version >= 3) {
            zwlr_output_head_v1_release(head->wl);
        } else {
            zwlr_output_head_v1_destroy(head->wl);
        }
        head->wl = nullptr;
    }
}

void OutputScaleManager::onHeadFinished(HeadInfo* head) {
    if (!head) return;
    head->finished = true;
    destroyHeadResources(head);
    m_heads.erase(
        std::remove_if(m_heads.begin(), m_heads.end(),
                       [head](const auto& ptr) { return ptr.get() == head; }),
        m_heads.end());
    if (m_onStateChanged) m_onStateChanged();
}

void OutputScaleManager::onModeFinished(ModeInfo* mode) {
    if (!mode || !mode->head) return;
    mode->finished = true;
    auto* head = mode->head;
    if (head->currentMode == mode->wl) {
        head->currentMode = nullptr;
    }
    if (mode->wl) {
        if (m_version >= 3) {
            zwlr_output_mode_v1_release(mode->wl);
        } else {
            zwlr_output_mode_v1_destroy(mode->wl);
        }
        mode->wl = nullptr;
    }
    head->modes.erase(
        std::remove_if(head->modes.begin(), head->modes.end(),
                       [mode](const auto& ptr) { return ptr.get() == mode; }),
        head->modes.end());
}

void OutputScaleManager::onManagerDone(uint32_t serial) {
    m_serial = serial;
    m_hasSerial = true;

    for (auto& head : m_heads) {
        if (head->currentMode) {
            for (const auto& m : head->modes) {
                if (m->wl == head->currentMode) {
                    head->width = m->width;
                    head->height = m->height;
                    head->refreshMhz = m->refreshMhz;
                    break;
                }
            }
        }
    }

    if (m_onStateChanged) m_onStateChanged();
    applyIfNeeded();
}

void OutputScaleManager::onManagerFinished() {
    if (m_inFlightConfig) {
        zwlr_output_configuration_v1_destroy(m_inFlightConfig);
        m_inFlightConfig = nullptr;
    }
    for (auto& head : m_heads) {
        destroyHeadResources(head.get());
    }
    m_heads.clear();
    if (m_mgr) {
        zwlr_output_manager_v1_destroy(m_mgr);
        m_mgr = nullptr;
    }
    m_hasSerial = false;
}

void OutputScaleManager::onConfigSucceeded(zwlr_output_configuration_v1* cfg) {
    if (cfg == m_inFlightConfig) {
        m_inFlightConfig = nullptr;
    }
    zwlr_output_configuration_v1_destroy(cfg);
    if (m_onStateChanged) m_onStateChanged();
}

void OutputScaleManager::onConfigFailed(zwlr_output_configuration_v1* cfg) {
    if (cfg == m_inFlightConfig) {
        m_inFlightConfig = nullptr;
    }
    zwlr_output_configuration_v1_destroy(cfg);
    m_failedPending = true;
    std::cerr << "[lightwm-wm] Output scale configuration failed\n";
}

void OutputScaleManager::onConfigCancelled(zwlr_output_configuration_v1* cfg) {
    if (cfg == m_inFlightConfig) {
        m_inFlightConfig = nullptr;
    }
    zwlr_output_configuration_v1_destroy(cfg);
}

void OutputScaleManager::applyIfNeeded() {
    if (!m_mgr || !m_hasSerial || m_inFlightConfig || m_failedPending || m_heads.empty()) {
        return;
    }

    bool anyChange = false;
    for (const auto& head : m_heads) {
        if (head->finished || !head->enabled || head->name.empty()) continue;
        auto it = m_desiredScales.find(head->name);
        if (it != m_desiredScales.end() && std::abs(head->scale - it->second) > 0.005) {
            anyChange = true;
            break;
        }
    }
    if (!anyChange) return;

    zwlr_output_configuration_v1* cfg = zwlr_output_manager_v1_create_configuration(m_mgr, m_serial);
    if (!cfg) return;

    for (const auto& head : m_heads) {
        if (head->finished) continue;
        if (!head->enabled) {
            zwlr_output_configuration_v1_disable_head(cfg, head->wl);
            continue;
        }
        zwlr_output_configuration_head_v1* headCfg = zwlr_output_configuration_v1_enable_head(cfg, head->wl);
        auto it = m_desiredScales.find(head->name);
        if (it != m_desiredScales.end() && std::abs(head->scale - it->second) > 0.005) {
            zwlr_output_configuration_head_v1_set_scale(headCfg, wl_fixed_from_double(it->second));
        }
    }

    m_inFlightConfig = cfg;
    zwlr_output_configuration_v1_add_listener(cfg, &kConfigListener, this);
    zwlr_output_configuration_v1_apply(cfg);
}

} // namespace lightwm::wm
