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

template<typename T>
void process(MessageInputBuffer& buffer) {
    T tmp(buffer);
    describeMessageHeader(tmp.header_.header);
    std::cout << tmp.intField_ << ':' << tmp.doubleField_ << '\n';
}

template<>
void process<testMsg2>(MessageInputBuffer& buffer) {
    testMsg2 tmp(buffer);
    std::cout << tmp.intField_ << ':' << tmp.doubleField_ << ':' << tmp.stringField_ << ':' << tmp.stringField2_ << '\n';
}

void processMessageInputBuffer(MessageInputBuffer& buffer) {
    auto msgType = buffer.getMessageType();
    //std::cout << msgType << std::endl;
    switch(msgType) {
        case testMessageTypeIdsEnum::testMsg1: process<testMsg1>(buffer); break;
        case testMessageTypeIdsEnum::testMsg2: process<testMsg2>(buffer); break;
        default: std::cout << "unrecognized message type: " << msgType << '\n'; break;
    }
}

void processBuffer(const char* buffer, const uint32_t len) {
    if (len > sizeof(MessageHeader)) {
        MessageHeader* header = (MessageHeader*)buffer;
        //describeMessageHeader(*header);
        if (len == header->getMessageSize()) {
            MessageInputBuffer input(buffer, len);
            processMessageInputBuffer(input);
        }
    }
}


int main() {
    struct testMsg1 t1(3, 5.1);
    struct testMsg2 t2(9, 6.3, "hello", "worldworldworldworldworldworld");
    describeGeneratedMsg(t1);
    describeGeneratedMsg(t2);
    processBuffer((const char*)&t1, sizeof(t1));
    processBuffer((const char*)&t2, sizeof(t2));
    /*
    GenericMessage msg;
    t1.toGenericMessage(msg);
    std::cout << msg.getMessageType() << std::endl;
    */
}
