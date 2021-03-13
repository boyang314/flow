#include "test.H"
#include <iostream>

void describeFieldInfo(const MessageFieldInfo& fieldInfo) {
    std::cout << "fieldId: " << fieldInfo.getFieldId() << " fieldType: " << (int)fieldInfo.getFieldType().type() << " len: " << fieldInfo.getFieldLength() << std::endl;
}

void describeMessageHeader(const MessageHeader& header) {
    std::cout << "msgSize: " << header.getMessageSize() << " msgType: " << header.getMessageType() << " num: " << header.getNumOfFields() << std::endl;
}

template<typename T>
void describeGeneratedMsg(const T& msg) {
    const MessageHeader& header = msg.header_.header;
    describeMessageHeader(header);
    std::cout << "headerSize: " << sizeof(msg.header_) << " headerHeader:" << sizeof(header) << " fieldInfoSize:" << sizeof(MessageFieldInfo) << std::endl;
    uint16_t numOfFields = header.getNumOfFields();
    MessageFieldInfo* fieldPos = (MessageFieldInfo*)((char*)&(msg.header_) + sizeof(header));
    for (uint16_t i=0; i < numOfFields; ++i) {
        describeFieldInfo(*fieldPos++);
    }
}

int main() {
    struct testMsg1 t1;
    struct testMsg2 t2;
    describeGeneratedMsg(t1);
    describeGeneratedMsg(t2);
}
