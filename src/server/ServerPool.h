//
// Created by Emil Ebert on 01.05.25.
//

#ifndef SERVERPOOL_H
#define SERVERPOOL_H

#include <poll.h>

#include <string>
#include <vector>
#include "Server.h"

class ServerPool {
  private:
    std::vector<Server> servers;
    std::atomic<bool> running;
    std::vector<struct pollfd> fds;

public:
    ServerPool();
    ~ServerPool();

    void loadConfig(const std::string &configFile);
    void start(void);
    void stop(void);

private:
  void serverLoop(void);
};



#endif //SERVERPOOL_H
