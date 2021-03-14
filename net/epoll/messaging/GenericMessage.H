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

    MessageFieldValue getFieldValue(const uint16_t fieldId) const {
        auto iter = fieldMap_.find(fieldId);
        return (iter != fieldMap_.end()) ? fields_[iter->second] : MessageFieldValue();
    }
    void setFieldValue(const uint16_t fieldId, const MessageFieldValue& value) {
        auto iter = fieldMap_.find(fieldId);
        if (iter == fieldMap_.end()) {
            fieldMap_[fieldId] = fields_.size();
            fields_.push_back(value);
        } else fields_[iter->second] = value;
    }

    //typed setter
    template<typename T>
    void set(uint16_t id, const T& val) { setFieldValue(id, MessageFieldValue(val)); }
    void setString(uint16_t id, const char* val, uint16_t len) { setFieldValue(id, MessageFieldValue(val, len)); }

private: 
    GenericMessage(const GenericMessage& other);
    GenericMessage& operator=(const GenericMessage& other);

    uint16_t messageType_;
    std::vector<MessageFieldValue> fields_;
    std::map<uint16_t, uint16_t> fieldMap_;
};

