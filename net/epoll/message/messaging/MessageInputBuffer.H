#pragma once

#include "MessageFieldInfo.H"
#include <string>

struct MessageInputBuffer {
    void skipField(MessageFieldInfo fieldInfo);
    const char* getBuffer();
    template<typename T> 
    T get();
    template<unsigned LEN> 
    std::string getString(uint16_t len, const char*);
    MessageFieldInfo nextFieldInfo();
    bool hasMoreFields();
    uint16_t getMessageType();
};
