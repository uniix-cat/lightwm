#include "wm/IpcServer.hpp"

#include <QJsonDocument>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace lightwm::wm {

namespace {

bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

} // namespace

IpcServer::IpcServer(CommandCallback onCommand)
    : m_onCommand(std::move(onCommand)) {}

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::start(const std::string& socketPath) {
    stop();
    m_socketPath = socketPath;

    m_listenFd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (m_listenFd < 0) {
        return false;
    }
    setNonBlocking(m_listenFd);

    ::unlink(m_socketPath.c_str());

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (m_socketPath.size() >= sizeof(addr.sun_path)) {
        ::close(m_listenFd);
        m_listenFd = -1;
        return false;
    }
    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(m_listenFd);
        m_listenFd = -1;
        return false;
    }

    if (::listen(m_listenFd, 16) < 0) {
        ::close(m_listenFd);
        m_listenFd = -1;
        return false;
    }
    return true;
}

void IpcServer::stop() {
    for (auto& c : m_clients) {
        if (c.fd >= 0) ::close(c.fd);
    }
    m_clients.clear();

    if (m_listenFd >= 0) {
        ::close(m_listenFd);
        m_listenFd = -1;
    }
    if (!m_socketPath.empty()) {
        ::unlink(m_socketPath.c_str());
        m_socketPath.clear();
    }
}

void IpcServer::appendPollFds(std::vector<pollfd>& fds) const {
    if (m_listenFd >= 0) {
        fds.push_back(pollfd{m_listenFd, POLLIN, 0});
    }
    for (const auto& c : m_clients) {
        fds.push_back(pollfd{c.fd, POLLIN, 0});
    }
}

void IpcServer::handlePollEvents(const std::vector<pollfd>& fds) {
    for (const auto& p : fds) {
        if (p.revents == 0) continue;
        if (p.fd == m_listenFd && (p.revents & POLLIN)) {
            acceptPending();
            continue;
        }
        for (size_t i = 0; i < m_clients.size(); ++i) {
            if (m_clients[i].fd == p.fd) {
                if (p.revents & POLLIN) {
                    readClient(i);
                } else if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    removeClient(i);
                }
                break;
            }
        }
    }
}

void IpcServer::acceptPending() {
    while (true) {
        int cfd = ::accept4(m_listenFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            break;
        }
        m_clients.push_back(Client{cfd, {}});

        if (!m_lastState.isEmpty()) {
            QByteArray line = QJsonDocument(m_lastState).toJson(QJsonDocument::Compact) + '\n';
            (void)::send(cfd, line.constData(), static_cast<size_t>(line.size()), MSG_NOSIGNAL);
        }
    }
}

void IpcServer::readClient(size_t index) {
    char buf[4096];
    bool eof = false;
    bool err = false;
    while (true) {
        ssize_t n = ::recv(m_clients[index].fd, buf, sizeof(buf), 0);
        if (n > 0) {
            m_clients[index].readBuf.append(buf, static_cast<size_t>(n));
        } else if (n == 0) {
            eof = true;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            err = true;
            break;
        }
    }

    auto& rb = m_clients[index].readBuf;
    size_t pos = 0;
    while ((pos = rb.find('\n')) != std::string::npos) {
        const std::string line = rb.substr(0, pos);
        rb.erase(0, pos + 1);
        if (line.empty()) continue;

        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(line));
        if (doc.isObject() && m_onCommand) {
            m_onCommand(doc.object());
        }
    }

    if (eof || err) {
        removeClient(index);
    }
}

void IpcServer::removeClient(size_t index) {
    if (index < m_clients.size()) {
        ::close(m_clients[index].fd);
        m_clients.erase(m_clients.begin() + static_cast<ptrdiff_t>(index));
    }
}

void IpcServer::broadcastState(const QJsonObject& state) {
    m_lastState = state;
    const QByteArray line = QJsonDocument(state).toJson(QJsonDocument::Compact) + '\n';
    for (size_t i = 0; i < m_clients.size();) {
        ssize_t sent = ::send(m_clients[i].fd, line.constData(), static_cast<size_t>(line.size()), MSG_NOSIGNAL);
        if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            removeClient(i);
        } else {
            ++i;
        }
    }
}

} // namespace lightwm::wm
