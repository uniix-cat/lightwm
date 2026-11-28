#pragma once

#include <QJsonArray>
#include <QMap>
#include <QString>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct wl_registry;
struct zwlr_output_configuration_v1;
struct zwlr_output_head_v1;
struct zwlr_output_manager_v1;
struct zwlr_output_mode_v1;

namespace lightwm::wm {

// Manages output fractional scaling via wlr-output-management-unstable-v1.
class OutputScaleManager {
public:
    using StateChangedCallback = std::function<void()>;

    struct HeadInfo;

    struct ModeInfo {
        zwlr_output_mode_v1* wl = nullptr;
        HeadInfo* head = nullptr;
        int32_t width = 0;
        int32_t height = 0;
        int32_t refreshMhz = 0;
        bool preferred = false;
        bool finished = false;
    };

    struct HeadInfo {
        OutputScaleManager* mgr = nullptr;
        zwlr_output_head_v1* wl = nullptr;
        std::string name;
        std::string description;
        bool enabled = false;
        double scale = 1.0;
        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 0;
        int32_t height = 0;
        int32_t refreshMhz = 0;
        zwlr_output_mode_v1* currentMode = nullptr;
        std::vector<std::unique_ptr<ModeInfo>> modes;
        bool finished = false;
    };

    explicit OutputScaleManager(StateChangedCallback onStateChanged = {});
    ~OutputScaleManager();

    void bind(wl_registry* registry, uint32_t name, uint32_t version);
    void setDesiredScales(const QMap<QString, double>& scales);
    bool isBound() const { return m_mgr != nullptr; }

    QJsonArray outputsJson() const;

    // Protocol callbacks
    void onNewHead(zwlr_output_head_v1* wlHead);
    void onHeadFinished(HeadInfo* head);
    void onModeFinished(ModeInfo* mode);
    void onManagerDone(uint32_t serial);
    void onManagerFinished();
    void onConfigSucceeded(zwlr_output_configuration_v1* cfg);
    void onConfigFailed(zwlr_output_configuration_v1* cfg);
    void onConfigCancelled(zwlr_output_configuration_v1* cfg);

private:
    void applyIfNeeded();
    void destroyHeadResources(HeadInfo* head);

    StateChangedCallback m_onStateChanged;
    zwlr_output_manager_v1* m_mgr = nullptr;
    uint32_t m_version = 0;
    uint32_t m_serial = 0;
    bool m_hasSerial = false;
    bool m_failedPending = false;
    zwlr_output_configuration_v1* m_inFlightConfig = nullptr;

    std::vector<std::unique_ptr<HeadInfo>> m_heads;
    std::unordered_map<std::string, double> m_desiredScales;
};

} // namespace lightwm::wm
