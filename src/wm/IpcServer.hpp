#pragma once

#include <QJsonObject>
#include <functional>
#include <string>
#include <vector>

struct pollfd;

namespace lightwm::wm {

class IpcServer {
public:
    using CommandCallback = std::function<void(const QJsonObject& cmd)>;

    explicit IpcServer(CommandCallback onCommand);
    ~IpcServer();

    bool start(const std::string& socketPath);
    void stop();

    int listenFd() const { return m_listenFd; }
    void appendPollFds(std::vector<pollfd>& fds) const;
    void handlePollEvents(const std::vector<pollfd>& fds);

    void broadcastState(const QJsonObject& state);

private:
    struct Client {
        int fd = -1;
        std::string readBuf;
    };

    void acceptPending();
    void readClient(size_t index);
    void removeClient(size_t index);

    CommandCallback m_onCommand;
    std::string m_socketPath;
    int m_listenFd = -1;
    std::vector<Client> m_clients;
    QJsonObject m_lastState;
};

} // namespace lightwm::wm
