#pragma once

#include <cstdint>

struct MessageHeader {
    MessageHeader() : headerDetail_(0) {}

    MessageHeader(const uint32_t msgSize, const uint16_t msgType, const uint16_t numOfFields) : headerDetail_((((uint64_t)msgSize) << 32) | (((uint64_t)msgType) << 16) | ((uint64_t)numOfFields)) {} 

    MessageHeader(const MessageHeader& other) : headerDetail_(other.headerDetail_) {}
    MessageHeader& operator=(const MessageHeader& other) { headerDetail_ = other.headerDetail_; return *this; }

    bool operator==(const MessageHeader& other) const { return headerDetail_ == other.headerDetail_; }
    bool operator!=(const MessageHeader& other) const { return headerDetail_ != other.headerDetail_; }

    uint32_t getMessageSize() const { return (uint32_t)(headerDetail_ >> 32); }
    uint16_t getMessageType() const { return (uint16_t)((headerDetail_ >> 16) & 0xffff); }
    uint16_t getNumOfFields() const { return (uint16_t)(headerDetail_ & 0xffff); }
    
    void setMessageSize(const uint32_t msgSize) { 
        headerDetail_ = (headerDetail_ & 0x00000000ffffffff) | (((uint64_t)msgSize) << 32); }
    void setMessageType(const uint16_t msgType) { 
        headerDetail_ = (headerDetail_ & 0xffffffff0000ffff) | (((uint64_t)msgType) << 16); }
    void setNumOfFields(const uint16_t numOfFields) { 
        headerDetail_ = (headerDetail_ & 0xffffffffffff0000) | ((uint64_t)numOfFields); }

    void setup(const uint16_t msgType, const uint16_t numOfFields) {
        headerDetail_ = (((uint64_t)msgType) << 16) | ((uint64_t)numOfFields); }
    
private:
    uint64_t headerDetail_;
} __attribute__((packed));

