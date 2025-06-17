#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

class SessionManager {
public:
    // Parse “Cookie:” header, return sessionId.  If a new session is created, isNew=true.
    static std::string getSessionId(const std::string &cookieHeader, bool &isNew);

    // record that sessionId uploaded “filename”
    static void addUploadedFile(const std::string &sessionId, const std::string &filename);

    // does sessionId own filename?
    static bool ownsFile(const std::string &sessionId, const std::string &filename);

    // Serialize all sessions to a binary file
    static void serialize(const std::string &filename);
    // Load all sessions from a binary file
    static void deserialize(const std::string &filename);

private:
    static std::unordered_map<std::string, std::vector<std::string>> sessions;
    static std::mutex mutex_;
};

#endif // SESSIONMANAGER_H