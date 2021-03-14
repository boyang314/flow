#pragma once

#include "MessageFieldInfo.H"
#include <string>

struct MessageInputBuffer {
    MessageInputBuffer(const char* buffer, const uint32_t len) : buffer_(buffer), bufferSize_(len), messageBuffer_(0), headerReadPosition_(0), dataReadPosition_(0), messageType_(0), currentFields_(0), numOfFields_(0) { initRead(); }
    ~MessageInputBuffer() {}

    void initRead();

    template<typename T> 
    T get() { return getValue<T>(); }
    template<unsigned LEN> 
    void getString(uint16_t len, const char*) {}
    const char* getString(uint16_t len) { return nullptr; }
    
    MessageFieldInfo nextFieldInfo() {
        ++currentFields_;
        MessageFieldInfo* messageFieldInfo = (MessageFieldInfo*)(buffer_+headerReadPosition_);
        headerReadPosition_ += sizeof(MessageFieldInfo);
        return *messageFieldInfo;
    }

    bool hasMoreFields() { return currentFields_ < numOfFields_; }
    void skipField(MessageFieldInfo fieldInfo) { dataReadPosition_ += fieldInfo.getFieldLength(); }
    uint16_t getMessageType() const { return messageType_; }
    uint16_t getNumOfFields() const { return numOfFields_; }
    const char* getBuffer() const { return messageBuffer_; }
    const char* getRawBuffer() const { return buffer_; }
    uint32_t getRawBufferSize() const { return bufferSize_; }
    
private:
    MessageInputBuffer(const MessageInputBuffer&);
    MessageInputBuffer& operator=(const MessageInputBuffer&);

    template<typename T>
    T getValue() {
        T* value = (T*)(buffer_+dataReadPosition_);
        dataReadPosition_ += sizeof(T);
        return *value;
    }

    const char* buffer_;
    uint32_t bufferSize_;
    const char* messageBuffer_;
    uint32_t headerReadPosition_;
    uint32_t dataReadPosition_;
    uint16_t messageType_;
    uint16_t currentFields_;
    uint16_t numOfFields_;
};
