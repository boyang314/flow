#include "MessageHeader.H"

std::ostream& operator <<(std::ostream& os, const MessageHeader& header) {
    os << "msgSize: " << header.getMessageSize() << " msgType: " << header.getMessageType() << " num: " << header.getNumOfFields();
}

