//
// Created by Emil Ebert on 08.05.25.
//

#include "FdHandler.h"
#include <common/Logger.h>

std::vector<pollfd> FdHandler::pollfds;
std::unordered_map<int, std::function<bool(int, short)> > FdHandler::fdCallbacks;
std::queue<pollfd> FdHandler::fdQueue;

void FdHandler::addFd(const int fd, const short events, const std::function<bool(int, short)> &callback) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = events;
    fdQueue.push(pfd);
    fdCallbacks[fd] = callback;
}

std::vector<pollfd>::iterator FdHandler::removeFd(const int fd) {
    const auto it = std::remove_if(pollfds.begin(), pollfds.end(), [fd](const pollfd &pfd) {
        return pfd.fd == fd;
    });

    fdCallbacks.erase(fd);

    if (it != pollfds.end()) {
        return pollfds.erase(it, pollfds.end());
    }
    return pollfds.end();
}


void FdHandler::pollFds() {
    while (!fdQueue.empty() && pollfds.size() < 1024) {
        pollfds.push_back(fdQueue.front());
        fdQueue.pop();
    }

    const int ret = poll(pollfds.data(), pollfds.size(), 100);
    if (ret < 0) {
        std::cerr << "Poll error: " << strerror(errno) << std::endl;
        return;
    }

    auto it = pollfds.begin();
    while (it != pollfds.end()) {
        if (it->revents & POLLERR) {
            std::cerr << "Poll error on fd: " << it->fd << std::endl;
            std::cerr << errno << ": " << strerror(errno) << std::endl;
            it = removeFd(it->fd);
            continue;
        }
        if (it->revents & POLLNVAL) {
            it = removeFd(it->fd);
            continue;
        }
        if (it->revents & POLLIN || it->revents & POLLOUT || it->revents & POLLHUP) {
            if (auto fdCallbacksIt = fdCallbacks.find(it->fd); fdCallbacksIt == fdCallbacks.end()) {
                it = removeFd(it->fd);
                continue;
            }

            try {
                if (fdCallbacks[it->fd](it->fd, it->revents)) {
                    it = removeFd(it->fd);
                    continue;
                }
            }catch (std::exception &e) {
                Logger::log(LogLevel::ERROR, e.what());
            }
        }
        ++it;
    }
}
