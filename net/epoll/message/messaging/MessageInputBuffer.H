#pragma once

#include "MessageFieldInfo.H"
#include <string>

struct MessageInputBuffer {

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
    uint16_t getMessageType() { return messageType_; }
    uint16_t getNumOfFields() { return numOfFields_; }
    const char* getBuffer() { return messageBuffer_; }
    const char* getRawBuffer() { return buffer_; }
    uint32_t getRawBufferSize() { return bufferSize_; }
    
private:
    //MessageInputbuffer(const MessageInputBuffer&);
    //MessageInputbuffer& operator=(const MessageInputBuffer&);

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
