//
// Created by Emil Ebert on 15.05.25.
//

#ifndef SMARTBUFFER_H
#define SMARTBUFFER_H

#include <sys/types.h>
#include <string>
#include <functional>

class SmartBuffer {
private:
    int fd = -1;
    size_t size = 0;
    size_t maxMemorySize = 0;
    bool isFile = false;
    std::string buffer;
    std::string writeBuffer;
    std::string readBuffer;
    size_t readPos = 0;
    size_t toRead = 0;
    bool fdCallbackRegistered = false;
    static size_t tmpFileCount;
    std::string tmpFileName;

public:
    SmartBuffer(size_t maxMemorySize = 40000);

    SmartBuffer(int fd);

    ~SmartBuffer();

    void switchToFile();

    bool onFileEvent(int fd, short events);

    void append(const char *data, size_t length);

    void read(size_t length);

    void unregisterCallback();

    void cleanReadBuffer(size_t length);

    [[nodiscard]] std::string getReadBuffer() const { return readBuffer; }
    [[nodiscard]] size_t getReadPos() const { return readPos; }
    [[nodiscard]] size_t getSize() const { return size; }
    [[nodiscard]] bool isFileBuffer() const { return isFile; }
    [[nodiscard]] int getFd() const { return fd; }
    [[nodiscard]] std::string getTmpFileName() const { return tmpFileName; }
    [[nodiscard]] bool isStillWriting() const { return !writeBuffer.empty(); }
};

#endif //SMARTBUFFER_H
