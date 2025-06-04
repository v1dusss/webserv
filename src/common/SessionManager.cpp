#include "SessionManager.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <iterator>

std::unordered_map<std::string,std::vector<std::string>> SessionManager::sessions;
std::mutex SessionManager::mutex_;

static std::string generateSessionId() {
    static std::random_device rd;
    static std::mt19937_64 eng(rd());
    static std::uniform_int_distribution<uint64_t> dist;
    uint64_t val = dist(eng);
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << val;
    return ss.str();
}

std::string SessionManager::getSessionId(const std::string &cookieHeader, bool &isNew) {
    std::lock_guard<std::mutex> lk(mutex_);
    isNew = false;
    // look for “sessionId=…”
    auto pos = cookieHeader.find("sessionId=");
    if (pos != std::string::npos) {
        pos += strlen("sessionId=");
        auto end = cookieHeader.find(';', pos);
        std::string id = cookieHeader.substr(pos, end-pos);
        if (sessions.count(id)) return id;
    }
    // else create
    std::string newId = generateSessionId();
    sessions[newId] = {};
    isNew = true;
    return newId;
}

void SessionManager::addUploadedFile(const std::string &sid,const std::string &fn){
    std::lock_guard<std::mutex> lk(mutex_);
    sessions[sid].push_back(fn);
}

bool SessionManager::ownsFile(const std::string &sid,const std::string &fn){
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions.find(sid);
    if (it==sessions.end()) return false;
    auto &v = it->second;
    return std::find(v.begin(), v.end(), fn)!=v.end();
}