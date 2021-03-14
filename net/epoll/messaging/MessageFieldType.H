#pragma once

#include <cstdint>

struct MessageFieldTypeEnum { enum TypeID { Unknown, _int, _double, _string, Max }; };

struct MessageFieldType {
    static const MessageFieldType Unknown;
    static const MessageFieldType _int;
    static const MessageFieldType _double;
    static const MessageFieldType _string;
    static const MessageFieldType Max;

    MessageFieldType() : typeId_(MessageFieldTypeEnum::Unknown) {}
    explicit MessageFieldType(const uint8_t typeId) : typeId_(typeId) {}
    MessageFieldType(const MessageFieldType& other) : typeId_(other.typeId_) {}
    MessageFieldType& operator=(const MessageFieldType& other) {
        typeId_ = other.typeId_;
        return *this;
    }

    bool operator==(const MessageFieldType& other) const { return typeId_ == other.typeId_; }
    bool operator!=(const MessageFieldType& other) const { return typeId_ != other.typeId_; }
    bool operator<(const MessageFieldType& other) const { return typeId_ < other.typeId_; }
    bool operator>(const MessageFieldType& other) const { return typeId_ > other.typeId_; }

    uint8_t type() const { return typeId_; }

    //helper isType()
    bool isUnknown() const { return *this == Unknown; }
    
private:
    uint8_t typeId_;
} __attribute__((packed));
