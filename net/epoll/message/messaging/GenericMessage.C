#include "GenericMessage.H"

void GenericMessage::fromMessageBuffer(MessageInputBuffer& inputBuffer) {
    while(inputBuffer.hasMoreFields()) {
        MessageFieldInfo fieldInfo = inputBuffer.nextFieldInfo();
        switch (fieldInfo.getFieldType().type()) {
        case MessageFieldTypeEnum::_int: set<int>(fieldInfo.getFieldId(), inputBuffer.get<int>()); break;
        case MessageFieldTypeEnum::_double: set<double>(fieldInfo.getFieldId(), inputBuffer.get<double>()); break;
        case MessageFieldTypeEnum::_string: setString(fieldInfo.getFieldId(), inputBuffer.getString(fieldInfo.getFieldLength()), fieldInfo.getFieldLength()); break;
        default: inputBuffer.skipField(fieldInfo); break;
        }
    }
}

void GenericMessage::toMessageBuffer(MessageOutputBuffer& outputBuffer, const uint16_t messageType) const {
    outputBuffer.setup(messageType, fields_.size());
    for (const auto& field : fields_) {
        switch(field.getFieldType().type()) {
        case MessageFieldTypeEnum::_int: break;
        case MessageFieldTypeEnum::_double: break;
        case MessageFieldTypeEnum::_string: break;
        default: break;
        }
    }
    outputBuffer.commit();
}

