#include "lwm2mResource.hpp"

#include <string.h>

#include "endian.hpp"

using namespace LwM2M;

void Resource::defaultOperation(Operations operation, TLVData* tlv, int dataLength) {}

/*
enum TLVType {
    ObjectInstance  = 0,
    ResouceInstance,
    MultipleResouce,
    Resouce
};
*/

Resource::Resource(int id, Operations operation, ResourceType type) {
	this->id		 = id;
	this->operation = operation;
	this->type	 = type;
	this->typeOfId	 = 3;  // LwM2M::TLVType::Resource

	callback = defaultOperation;

	switch (type) {
		case TypeString:
		case TypeOpaque:
		case TypeNone:
			data.bytesValue    = (uint8_t*)malloc(256); // 4096くらいとってもいい
			data.bytesValue[0] = '\0';
			data.dataLength    = 0;
			break;
		case TypeInteger:
		case TypeBoolean:
			data.byteValue	 = 0;
			data.dataLength = 1;
			break;
		case TypeFloat:
		case TypeTime:
			data.floatValue = 0;
			data.longValue	 = 0;
			data.dataLength = 8;
			break;
		case TypeObjlnk:
			data.objectInstanceLinkValue = 0;
			data.dataLength		    = 4;
			break;
	}
}

void Resource::registCallback(std::function<void(Operations operation, TLVData* tlv, int dataLength)> callback) {
	this->callback = callback;
}

bool Resource::isRead() { return operation & Read; }
bool Resource::isWrite() { return operation & Write; }
bool Resource::isExecute() { return operation & Execute; }

int Resource::Serialize(uint8_t* buffer) {
	buffer[0] = typeOfId << 6;

	int length = 1;
	if (id <= 0xff) {
		buffer[length++] = (uint8_t)id;
	} else {
		buffer[0] += (1 << 5);
		length += putUint16ToBytes(id, &buffer[length]);
	}

	if (data.dataLength <= 0x07) {
		buffer[0] += (uint8_t)data.dataLength;
	} else if (data.dataLength <= 0xff) {
		buffer[0] += (1 << 3);
		buffer[length++] = (uint8_t)data.dataLength;
	} else if (data.dataLength <= 0xffff) {
		buffer[0] += (2 << 3);
		length += putUint16ToBytes((uint16_t)data.dataLength, &buffer[length]);
	} else {
		buffer[0] += (3 << 3);
		length += putUint24ToBytes((uint32_t)data.dataLength, &buffer[length]);
	}

	if (data.dataLength == 0) return length;

	switch (type) {
		case TypeInteger:
		case TypeTime:
			switch (data.dataLength) {
				case 1:
					buffer[length++] = data.byteValue;
					break;
				case 2:
					length += putUint16ToBytes(data.wordValue, &buffer[length]);
					break;
				case 4:
					length += putUint32ToBytes(data.shortValue, &buffer[length]);
					break;
				case 8:
					length += putUint64ToBytes(data.longValue, &buffer[length]);
					break;
			}
			break;
		case TypeFloat:
			length += putUint64ToBytes(data.floatValue, &buffer[length]);
			break;
		case TypeBoolean:
			buffer[length++] = (uint8_t)data.byteValue;
			break;
		case TypeObjlnk:
			length += putUint32ToBytes(data.objectInstanceLinkValue, &buffer[length]);
			break;
		case TypeString:
		case TypeOpaque:
		case TypeNone:
			memcpy(&buffer[length], data.bytesValue, data.dataLength);
			length += data.dataLength;
		default:
			break;
	}

	return length;
}

uint16_t Resource::getTLVId(const uint8_t* buffer) {
	return buffer[0] & 0x20 ? getUint16FromBytes(&buffer[1]) : buffer[1];
}

int Resource::parse(const uint8_t* buffer) {
	int index = 0;

	typeOfId = (buffer[index++] >> 6);
	id	    = getTLVId(buffer);
	index += buffer[0] & 0x20 ? 2 : 1;

	switch ((buffer[0] >> 3) & 0x03) {
		case 0:
			data.dataLength = buffer[0] & 0x07;
			break;
		case 1:
			data.dataLength = buffer[index++];
			break;
		case 2:
			data.dataLength = getUint16FromBytes(&buffer[index++]);
			index += 2;
			break;
		case 3:
			data.dataLength = getUint24FromBytes(&buffer[index++]);
			index += 3;
			break;
	}

	if (data.dataLength == 0) return index;

	switch (type) {
		case TypeInteger:
		case TypeTime:
			switch (data.dataLength) {
				case 1:
					data.byteValue = (int8_t)buffer[index];
					break;
				case 2:
					data.wordValue = (int16_t)getUint16FromBytes(&buffer[index]);
					break;
				case 3:
					data.shortValue = (int32_t)getUint32FromBytes(&buffer[index]);
					break;
				case 4:
					data.longValue = (int64_t)getUint64FromBytes(&buffer[index]);
			}
			break;
		case TypeFloat:
			data.floatValue = (double)(getFloat64FromBytes(&buffer[index]));
			break;
		case TypeBoolean:
			data.byteValue = buffer[index];
			break;
		case TypeObjlnk:
			data.objectInstanceLinkValue = getUint16FromBytes(&buffer[index]);
			data.objectInstanceLinkValue <<= 16;
			data.objectInstanceLinkValue += getUint16FromBytes(&buffer[index + 2]);
			break;
		case TypeString:
		case TypeOpaque:
		case TypeNone:
			memcpy(data.bytesValue, &buffer[index], data.dataLength);
			break;
	}

	return index + data.dataLength;
}

int Resource::calculateParseLength(const uint8_t* buffer) {
	int index = 0;

	index += buffer[0] & 0x20 ? 2 : 1;

	int dataLength = 0;
	switch ((buffer[0] >> 3) & 0x03) {
		case 0:
			dataLength = buffer[0] & 0x07;
			break;
		case 1:
			dataLength = buffer[index++];
			break;
		case 2:
			dataLength = getUint16FromBytes(&buffer[index++]);
			index += 2;
			break;
		case 3:
			dataLength = getUint24FromBytes(&buffer[index++]);
			index += 3;
			break;
	}

	return index + dataLength;
}

void Resource::read() { callback(Read, &data, data.dataLength); }

void Resource::update() { callback(Write, &data, data.dataLength); }

void Resource::execute(const uint8_t* args, int length) {
	TLVData data;
	data.bytesValue = (uint8_t*)malloc(length + 1);
	memcpy(data.bytesValue, args, length);
	data.bytesValue[length] = '\0';
	callback(Execute, &data, length);
	free(data.bytesValue);
}
