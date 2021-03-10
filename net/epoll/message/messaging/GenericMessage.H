#pragma once

#include <vector>
#include <map>
#include "MessageFieldValue.H"
#include "MessageInputBuffer.H"
#include "MessageOutputBuffer.H"

struct GenericMessage {
    GenericMessage() : messageType_(0), fields_(), fieldMap_() {}
    GenericMessage(MessageInputBuffer& messageBuffer) : messageType_(), fields_(), fieldMap_() {
        fromMessageBuffer(messageBuffer);
    }

    ~GenericMessage() {}

    void fromMessageBuffer(MessageInputBuffer&);
    void toMessageBuffer(MessageOutputBuffer&, const uint16_t messageType) const;
    
    uint16_t getMessageType() const { return messageType_; }
    void setMessageType(const uint16_t messageType) { messageType_ = messageType; }

    MessageFieldValue getFieldValue(const uint16_t fieldId) const;
    void setFieldValue(const uint16_t fieldId, const MessageFieldValue& value);

    //typed setter
    template<typename T>
    void set(uint16_t id, const T& val);
    void setString(uint16_t id, const char*, uint16_t len);

private: 
    GenericMessage(const GenericMessage& other);
    GenericMessage& operator=(const GenericMessage& other);

    uint16_t messageType_;
    std::vector<MessageFieldValue> fields_;
    std::map<uint16_t, uint16_t> fieldMap_;
};

