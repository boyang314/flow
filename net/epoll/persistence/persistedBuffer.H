#pragma once

#include <cstdint>

class persistedBuffer {

public:
    persistedBuffer(uint32_t defaultBufferSize);
    ~persistedBuffer();
    uint32_t add(char *buf, uint16_t size);
    uint32_t getSize() { return writePos_; }
    uint32_t getNumOfMsgs() { return msgCount_; }
    void clean();

private:
    persistedBuffer(const persistedBuffer&);
    persistedBuffer& operator=(const persistedBuffer&);

    void resize();
    char* messageBuffer_;
    uint32_t messageBufferSize_;
    uint32_t msgCount_;
    uint32_t writePos_;
    int fd_;
};
