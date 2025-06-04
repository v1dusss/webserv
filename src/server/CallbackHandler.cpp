//
// Created by Emil Ebert on 04.06.25.
//

#include "CallbackHandler.h"

std::unordered_map<int, std::function<bool()>> CallbackHandler::callbacks;

size_t CallbackHandler::registerCallback(const std::function<bool()> &callback) {
    static int nextId = 0;
    callbacks[nextId] = callback;
    return nextId++;
}

void CallbackHandler::unregisterCallback(const size_t id) {
    auto it = callbacks.find(id);
    if (it != callbacks.end()) {
        callbacks.erase(it);
    }
}

void CallbackHandler::executeCallbacks() {
    for (auto it = callbacks.begin(); it != callbacks.end();) {
        if (it->second())
            it = callbacks.erase(it);
        else
            ++it;

    }
}