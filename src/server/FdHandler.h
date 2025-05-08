//
// Created by Emil Ebert on 08.05.25.
//

#ifndef FDHANDLER_H
#define FDHANDLER_H

#include <iostream>
#include <memory>
#include <vector>
#include <poll.h>
#include <unordered_map>
#include <functional>
#include <queue>
#include <cerrno>
#include <cstring>


class FdHandler {
private:
    static std::vector<pollfd> pollfds;
    static std::unordered_map<int, std::function<bool(int, short)> > fdCallbacks;
    static std::queue<pollfd> fdQueue;

public:
    static void addFd(int fd, short events, const std::function<bool(int, short)> &callback);
    static void removeFd(int fd);

    static void pollFds();
};


#endif //FDHANDLER_H
