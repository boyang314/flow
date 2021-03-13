#include "MessageInputBuffer.H"
#include "MessageHeader.H"

void MessageInputBuffer::initRead() {
    headerReadPosition_ = sizeof(MessageHeader);
    messageBuffer_ = buffer_; //in case we need to stuff subject into header
    MessageHeader* messageHeader = (MessageHeader*)messageBuffer_;
    messageType_ = messageHeader->getMessageType();
    numOfFields_ = messageHeader->getNumOfFields();
    dataReadPosition_ = sizeof(MessageHeader) + numOfFields_ * sizeof(MessageFieldInfo);
}
