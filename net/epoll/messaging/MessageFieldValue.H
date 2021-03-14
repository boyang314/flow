#pragma once

#include "MessageFieldInfo.H"

union FieldValue {
    int intValue;
    double doubleValue;
    const char* stringValue;
};

struct MessageFieldValue {
    MessageFieldValue();
    //template<typename T>
    //MessageFieldValue(const T val) : fieldInfo_(), fieldValue_() {}
    MessageFieldValue(const int val) : fieldInfo_(), fieldValue_() {}
    MessageFieldValue(const double val) : fieldInfo_(), fieldValue_() {}
    MessageFieldValue(const char* val, uint16_t len) : fieldInfo_(), fieldValue_() {}
    MessageFieldType getFieldType() const { return fieldInfo_.getFieldType(); }
private:
    MessageFieldInfo fieldInfo_;
    FieldValue fieldValue_;
};
