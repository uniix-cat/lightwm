#include "core/IpcProtocol.hpp"
#include "wm/WindowManager.hpp"

#include <csignal>
#include <iostream>

namespace {
lightwm::wm::WindowManager* g_wm = nullptr;

void handleSignal(int) {
    if (g_wm) {
        g_wm->requestQuit();
    }
}
} // namespace

int main() {
    lightwm::wm::WindowManager wm;
    g_wm = &wm;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const std::string sock = lightwm::ipc::socketPath().toStdString();
    if (!wm.init(sock)) {
        std::cerr << "[lightwm-wm] Initialization failed\n";
        return 1;
    }

    return wm.run();
}
