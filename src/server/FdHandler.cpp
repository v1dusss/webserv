//
// Created by Emil Ebert on 08.05.25.
//

#include "FdHandler.h"

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

void FdHandler::removeFd(const int fd) {
    fdCallbacks.erase(fd);
    const auto it = std::remove_if(pollfds.begin(), pollfds.end(), [fd](const pollfd &pfd) {
        return pfd.fd == fd;
    });
    if (it != pollfds.end()) {
        pollfds.erase(it, pollfds.end());
    }
}

void FdHandler::pollFds() {
    while (!fdQueue.empty() && pollfds.size() < 1024) {
        pollfds.push_back(fdQueue.front());
        fdQueue.pop();
    }

    const int ret = poll(pollfds.data(), pollfds.size(), 0);
    if (ret < 0) {
        std::cerr << "Poll error: " << strerror(errno) << std::endl;
        return;
    }

    auto it = pollfds.begin();
    while (it != pollfds.end()) {
        if (it->revents & POLLERR) {
            std::cerr << "Poll error on fd: " << it->fd << std::endl;
            it = pollfds.erase(it);
            continue;
        }
        if (it->revents & POLLNVAL) {
            std::cerr << "Poll invalid fd: " << it->fd << std::endl;
            it = pollfds.erase(it);
            continue;
        }
        if (it->revents & POLLIN || it->revents & POLLOUT)
            if (fdCallbacks[it->fd](it->fd, it->revents)) {
                it = pollfds.erase(it);
                continue;
            }
        ++it;
    }
}
