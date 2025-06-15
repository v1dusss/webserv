//
// Created by Emil Ebert on 04.06.25.
//

#ifndef CALLBACKHANDLER_H
#define CALLBACKHANDLER_H

#include <unordered_map>
#include <functional>

class CallbackHandler {
  private:
    static std::unordered_map<int, std::function<bool()>> callbacks;

    public:
      static size_t registerCallback(const std::function<bool()> &callback);
       static void unregisterCallback(const size_t id);
       static void executeCallbacks();
};



#endif //CALLBACKHANDLER_H
