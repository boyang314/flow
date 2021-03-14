#pragma once

#include "GenericMessage.H"
#include "MessageInputBuffer.H"
#include "MessageHeader.H"
#include "MessageFieldInfo.H"
#include <stdint.h>
#include <string.h>
struct testMessageFiledIdsEnum {
    enum TypeId {
	intField = 100,
	doubleField = 101,
	stringField = 102,
	stringField2 = 103,
    };
};
struct testMessageTypeIdsEnum {
    enum TypeId {
	testMsg1 = 103,
	testMsg2 = 104,
    };
};
struct testMsg1Header {
	testMsg1Header(const uint32_t messageSize) : header(messageSize, testMessageTypeIdsEnum::testMsg1, 2), intField(testMessageFiledIdsEnum::intField, MessageFieldTypeEnum::_int, sizeof(int)), doubleField(testMessageFiledIdsEnum::doubleField, MessageFieldTypeEnum::_double, sizeof(double)) {}
	bool operator==(const testMsg1Header &other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
	MessageHeader header;
	MessageFieldInfo intField;
	MessageFieldInfo doubleField;
} __attribute__((packed));
struct testMsg2Header {
	testMsg2Header(const uint32_t messageSize) : header(messageSize, testMessageTypeIdsEnum::testMsg2, 4), intField(testMessageFiledIdsEnum::intField, MessageFieldTypeEnum::_int, sizeof(int)), doubleField(testMessageFiledIdsEnum::doubleField, MessageFieldTypeEnum::_double, sizeof(double)), stringField(testMessageFiledIdsEnum::stringField, MessageFieldTypeEnum::_string, 32), stringField2(testMessageFiledIdsEnum::stringField2, MessageFieldTypeEnum::_string, 28) {}
	bool operator==(const testMsg2Header &other) const { return memcmp(this, &other, sizeof(*this)) == 0; }
	MessageHeader header;
	MessageFieldInfo intField;
	MessageFieldInfo doubleField;
	MessageFieldInfo stringField;
	MessageFieldInfo stringField2;
} __attribute__((packed));
struct testMsg1 {
	testMsg1() : header_(sizeof(*this)), intField_(), doubleField_() {}
	testMsg1(int intField, double doubleField) : header_(sizeof(*this)), intField_(intField), doubleField_(doubleField) {}
	testMsg1(MessageInputBuffer& messageBuffer) : header_(sizeof(*this)), intField_(), doubleField_() { fromMessageBuffer(messageBuffer); }
	static bool canCast(MessageInputBuffer& messageBuffer) {
		const char* buffer = messageBuffer.getBuffer();
		static testMsg1Header staticHeader(sizeof(testMsg1));
		return staticHeader == *((testMsg1Header*)buffer);
	}
	static bool typeMatch(MessageInputBuffer& messageBuffer) {
		return messageBuffer.getMessageType() == testMessageTypeIdsEnum::testMsg1;
	}
	void toGenericMessage(GenericMessage& message) const {
		message.set<int>(100, intField_);
		message.set<double>(101, doubleField_);
	}
	void fromMessageBuffer(MessageInputBuffer& messageBuffer) {
		const char* buffer = messageBuffer.getBuffer();
		testMsg1Header* msgHeader = (testMsg1Header*)buffer;
		if (header_ == *msgHeader) {
			memcpy((char*)this, buffer, sizeof(*this));
		} else {
			while (messageBuffer.hasMoreFields()) {
				MessageFieldInfo fieldInfo = messageBuffer.nextFieldInfo();
				switch (fieldInfo.getFieldId()) {
				case testMessageFiledIdsEnum::intField:
					if (fieldInfo.getFieldType() == MessageFieldType::_int) intField_ = messageBuffer.get<int>();
					else messageBuffer.skipField(fieldInfo);
					break;
				case testMessageFiledIdsEnum::doubleField:
					if (fieldInfo.getFieldType() == MessageFieldType::_double) doubleField_ = messageBuffer.get<double>();
					else messageBuffer.skipField(fieldInfo);
					break;
				default:
					messageBuffer.skipField(fieldInfo);
					break;
				}
			}
		}
	}
	testMsg1Header header_;
	int intField_;
	double doubleField_;
} __attribute__((packed));
struct testMsg2 {
	testMsg2() : header_(sizeof(*this)), intField_(), doubleField_(), stringField_(), stringField2_() {}
	testMsg2(int intField, double doubleField, const char* stringField, const char* stringField2) : header_(sizeof(*this)), intField_(intField), doubleField_(doubleField), stringField_(), stringField2_() {
	strncpy(stringField_, stringField, 31);
	stringField_[31] = '\0'; 
	strncpy(stringField2_, stringField2, 27);
	stringField2_[27] = '\0'; }
	testMsg2(MessageInputBuffer& messageBuffer) : header_(sizeof(*this)), intField_(), doubleField_(), stringField_(), stringField2_() { fromMessageBuffer(messageBuffer); }
	static bool canCast(MessageInputBuffer& messageBuffer) {
		const char* buffer = messageBuffer.getBuffer();
		static testMsg2Header staticHeader(sizeof(testMsg2));
		return staticHeader == *((testMsg2Header*)buffer);
	}
	static bool typeMatch(MessageInputBuffer& messageBuffer) {
		return messageBuffer.getMessageType() == testMessageTypeIdsEnum::testMsg2;
	}
	void toGenericMessage(GenericMessage& message) const {
		message.set<int>(100, intField_);
		message.set<double>(101, doubleField_);
		message.setString(102, stringField_, 32);
		message.setString(103, stringField2_, 28);
	}
	void fromMessageBuffer(MessageInputBuffer& messageBuffer) {
		const char* buffer = messageBuffer.getBuffer();
		testMsg2Header* msgHeader = (testMsg2Header*)buffer;
		if (header_ == *msgHeader) {
			memcpy((char*)this, buffer, sizeof(*this));
		} else {
			while (messageBuffer.hasMoreFields()) {
				MessageFieldInfo fieldInfo = messageBuffer.nextFieldInfo();
				switch (fieldInfo.getFieldId()) {
				case testMessageFiledIdsEnum::intField:
					if (fieldInfo.getFieldType() == MessageFieldType::_int) intField_ = messageBuffer.get<int>();
					else messageBuffer.skipField(fieldInfo);
					break;
				case testMessageFiledIdsEnum::doubleField:
					if (fieldInfo.getFieldType() == MessageFieldType::_double) doubleField_ = messageBuffer.get<double>();
					else messageBuffer.skipField(fieldInfo);
					break;
				case testMessageFiledIdsEnum::stringField:
					if (fieldInfo.getFieldType() == MessageFieldType::_string) messageBuffer.getString<32>(fieldInfo.getFieldLength(), stringField_);
					else messageBuffer.skipField(fieldInfo);
					break;
				case testMessageFiledIdsEnum::stringField2:
					if (fieldInfo.getFieldType() == MessageFieldType::_string) messageBuffer.getString<28>(fieldInfo.getFieldLength(), stringField2_);
					else messageBuffer.skipField(fieldInfo);
					break;
				default:
					messageBuffer.skipField(fieldInfo);
					break;
				}
			}
		}
	}
	testMsg2Header header_;
	int intField_;
	double doubleField_;
	char stringField_[32];
	char stringField2_[28];
} __attribute__((packed));
