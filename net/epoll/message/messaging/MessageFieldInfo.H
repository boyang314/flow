#pragma once

#include "MessageFieldType.H"

struct MessageFieldInfo {
    MessageFieldInfo() : fieldInfo_(0) {}
    explicit MessageFieldInfo(const uint32_t fieldInfo) : fieldInfo_(fieldInfo) {}
    explicit MessageFieldInfo(const uint16_t fieldId, const MessageFieldTypeEnum::TypeID fieldType, const uint16_t fieldLength) : fieldInfo_((fieldId << 16) | ((fieldLength & 0xfff) << 4) | (fieldType & 0xf)) {}

    MessageFieldInfo(const MessageFieldInfo& other) : fieldInfo_(other.fieldInfo_) {}
    MessageFieldInfo& operator=(const MessageFieldInfo& other) { fieldInfo_ = other.fieldInfo_; return *this; }

    bool operator==(const MessageFieldInfo& other) const { return fieldInfo_ == other.fieldInfo_; }
    bool operator!=(const MessageFieldInfo& other) const { return fieldInfo_ != other.fieldInfo_; }
    bool operator>(const MessageFieldInfo& other) const { return fieldInfo_ > other.fieldInfo_; }
    bool operator<(const MessageFieldInfo& other) const { return fieldInfo_ < other.fieldInfo_; }

    bool empty() const { return fieldInfo_ == 0; }
    uint16_t getFieldId() const { return fieldInfo_ >> 16; }
    uint16_t getFieldLength() const { return (fieldInfo_ >> 4) & 0xfff; }
    MessageFieldType getFieldType() const { return MessageFieldType((uint8_t)(fieldInfo_ & 0xf)); }
    uint32_t getFieldInfo() const { return fieldInfo_; }

    template<uint16_t FIELDID, uint8_t FIELDTYPE, uint16_t FIELDLENGTH>
    void setupField() { fieldInfo_ = (FIELDID << 16) | ((FIELDLENGTH & 0xfff) << 4) | (FIELDTYPE & 0xf); }

private:
    uint32_t fieldInfo_;
} __attribute__((packed));











