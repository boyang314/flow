#pragma once

#include <string.h>
#include "MessageHeader.H"

struct MessageOutputBuffer {
    MessageOutputBuffer(uint32_t bufferSize) : buffer_(new char[bufferSize]), bufferSize_(bufferSize), headerWritePosition_(0), dataWritePosition_(0), currentFields_(0), numOfFields_(0) {}
    ~MessageOutputBuffer() { delete[] buffer_; }

    void setup(const uint16_t msgType, const uint16_t numOfFields) {
        //figure out right buffer size;
        const uint32_t minimumSize = sizeof(MessageHeader) + numOfFields * sizeof(MessageFieldInfo);
        if (bufferSize_ < minimumSize) {
            while(bufferSize_ < minimumSize) bufferSize_ *= 2;
            delete[] buffer_;
            buffer_ = new char[bufferSize_];
        }
        currentFields_ = 0;
        numOfFields_ = numOfFields;
        ((MessageHeader*)buffer_)->setup(msgType, numOfFields);
        headerWritePosition_ = sizeof(MessageHeader);
        dataWritePosition_ = sizeof(MessageHeader) + numOfFields * sizeof(MessageFieldInfo);
    }

    void commit() {
        if (currentFields_ < numOfFields_) {
            //memmove
            const uint32_t dataStartPosition = sizeof(MessageHeader) + numOfFields_ * sizeof(MessageFieldInfo);
            const uint32_t headerEndPosition = sizeof(MessageHeader) + currentFields_ * sizeof(MessageFieldInfo);
            memmove(buffer_+headerEndPosition, buffer_+dataStartPosition, dataWritePosition_ - dataStartPosition);
            dataWritePosition_ -= (dataStartPosition - headerEndPosition);
            numOfFields_ = currentFields_;
            ((MessageHeader*)buffer_)->setNumOfFields(numOfFields_);
        }
        ((MessageHeader*)buffer_)->setMessageSize(dataWritePosition_);
    }

    //individual write function

    char* buffer_;
    uint32_t bufferSize_;
    uint32_t headerWritePosition_;
    uint32_t dataWritePosition_;
    uint32_t currentFields_;
    uint32_t numOfFields_;
};
