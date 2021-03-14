#pragma once

#include "epollservice/epollservice.H"
#include "messaging/MessageHeader.H"
#include "messaging/MessageInputBuffer.H"
#include "messaging/test.H"
#include <assert.h>

#define BUFSZ 65536

    template<typename T>
    void process(MessageInputBuffer& buffer) {
        T tmp(buffer);
        std::cout << tmp.intField_ << ':' << tmp.doubleField_ << '\n';
    }

    template<>
    void process<testMsg2>(MessageInputBuffer& buffer) {
        testMsg2 tmp(buffer);
        std::cout << tmp.intField_ << ':' << tmp.doubleField_ << ':' << tmp.stringField_ << ':' << tmp.stringField2_ << '\n';
    }

    void handleMessageInputBuffer(MessageInputBuffer& input) {
        MessageHeader* header = (MessageHeader*)(input.getRawBuffer());
        auto msgType = header->getMessageType();
        //std::cout << "message type: " << msgType << '\n';
        switch(msgType) {
            case testMessageTypeIdsEnum::testMsg1: process<testMsg1>(input); break;
            case testMessageTypeIdsEnum::testMsg2: process<testMsg2>(input); break;
            default: std::cout << "unrecognized message type: " << msgType << '\n'; break;
        }
    }

struct DatagramHandler : public DatagramListener {
    DatagramHandler() {
        memset(buffer_, 0, BUFSZ);
        bufferEnd_ = 0;
    }
    virtual void onPacket(const char* data, size_t len, const sockaddr_in& from) override {
        if (bufferEnd_+len > BUFSZ) {
            std::cout << "need to increase buffer size\n";
            return;
        }

        memcpy(buffer_+bufferEnd_, data, len);
        bufferEnd_ += len;

        uint16_t readPos = 0;
        while(bufferEnd_ > readPos + sizeof(MessageHeader)) {
            MessageHeader* header = (MessageHeader*)(buffer_ + readPos);
            uint16_t sz = header->getMessageSize();
            if (readPos + sz <= bufferEnd_) {
                char* messageBuf = new char[sz];
                memmove(messageBuf, buffer_+readPos, sz);
                MessageInputBuffer input(messageBuf, sz);
                handleMessageInputBuffer(input);
                readPos += sz;
            } else {
                return;
            }
        }
        assert(readPos <= bufferEnd_);
        if (readPos == bufferEnd_) {
            bufferEnd_ = 0;
            memset(buffer_, 0, BUFSZ);
        } else if (readPos > 0) {
            bufferEnd_ -= readPos;
            memmove(buffer_, buffer_+readPos, bufferEnd_);
        }
    }
private:
    char buffer_[BUFSZ];
    uint16_t bufferEnd_;
};

struct MessageHandler : public StreamListener {
    MessageHandler() {
        memset(buffer_, 0, BUFSZ);
        bufferEnd_ = 0;
    }
    virtual void onPacket(const char* data, size_t len, const sockaddr_in& from, TcpConnection* conn) override {
        if (bufferEnd_+len > BUFSZ) {
            std::cout << "need to increase buffer size\n";
            return;
        }

        memcpy(buffer_+bufferEnd_, data, len);
        bufferEnd_ += len;

        uint16_t readPos = 0;
        while(bufferEnd_ > readPos + sizeof(MessageHeader)) {
            MessageHeader* header = (MessageHeader*)(buffer_ + readPos);
            uint16_t sz = header->getMessageSize();
            if (readPos + sz <= bufferEnd_) {
                char* messageBuf = new char[sz];
                memmove(messageBuf, buffer_+readPos, sz);
                MessageInputBuffer input(messageBuf, sz);
                handleMessageInputBuffer(input);
                readPos += sz;
            } else {
                return;
            }
        }
        assert(readPos <= bufferEnd_);
        if (readPos == bufferEnd_) {
            bufferEnd_ = 0;
            memset(buffer_, 0, BUFSZ);
        } else if (readPos > 0) {
            bufferEnd_ -= readPos;
            memmove(buffer_, buffer_+readPos, bufferEnd_);
        }
    }
private:
    char buffer_[BUFSZ];
    uint16_t bufferEnd_;
};
