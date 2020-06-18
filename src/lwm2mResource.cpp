#include "lwm2mResource.hpp"

#include <string.h>

using namespace LwM2M;

void Resource::nullOperation(Operations operation, TLVData* tlv) {}

Resource::Resource(uint16_t id, Operations operation, Type type) {
	this->operation = operation;
	this->type	 = type;

	memset(&header, 0, sizeof(header));
	memset(&data, 0, sizeof(data));

	header.id			    = id;
	header.meta.typeOfId    = TLVType::Resouce;
	header.meta.maxIdLength = id <= 0xff ? MaxIdLength::IdLength_FF : MaxIdLength::IdLength_FFFF;

	callback = nullOperation;
}

bool Resource::isRead() { return operation & Read; }
bool Resource::isWrite() { return operation & Write; }
bool Resource::isExecute() { return operation & Execute; }

void revcpy(uint8_t* output, const uint8_t* input, size_t length) {
	size_t i = 0;
	while (length > 0) output[i++] = input[--length];
}

int Resource::Serialize(uint8_t* buffer) {
	header.dataLength = getDataLength();

	if (header.dataLength > 0xffff) {
		header.meta.maxDataLength = MaxDataLength::DataLength_FFFFFF;
	} else if (header.dataLength > 0xff) {
		header.meta.maxDataLength = MaxDataLength::DataLength_FFFF;
	} else if (header.dataLength > 0x07) {
		header.meta.maxDataLength = MaxDataLength::DataLength_FF;
	} else {
		header.meta.maxDataLength = MaxDataLength::DataLength_07;
		header.meta.dataLength	 = header.dataLength;
		header.dataLength		 = 0;
	}

	// header.idは変更がおきない。
	// header.meta.maxIdLength = header.id > 0xff ? MaxIdLength::IdLength_FFFF : MaxIdLength::IdLength_FF;

	size_t b = 0;
	int h    = 6;
	// meta
	buffer[b++] = header.row[h--];
	// id
	if (header.row[h] != 0)
		buffer[b++] = header.row[h--];
	else
		h--;
	buffer[b++] = header.row[h--];

	// data length
	while (header.row[h] == 0) h--;
	while (h >= 0) buffer[b++] = header.row[h--];

	size_t len = header.dataLength ? header.dataLength : header.meta.dataLength;

	switch (type) {
		case Corelnk:
		case String:
		case Opaque:
			memcpy(buffer + b, data.bytesValue.pointer, len);
			break;
		default:
			revcpy(buffer + b, data.row, len);
			break;
	}

	return b + len;
}

size_t Resource::Deserialize(const uint8_t* buffer) { return parse(this, buffer); }

size_t Resource::parse(Resource* resource, const uint8_t* buffer) {
	TLVHeader _header;
	TLVHeader* header = resource ? &resource->header : &_header;

	size_t b = 0;
	int h    = 6;

	header->row[h--] = buffer[b++];

	// id
	header->row[h--] = header->meta.maxIdLength == MaxIdLength::IdLength_FF ? 0 : buffer[b++];
	header->row[h--] = buffer[b++];

	// datalength

	switch (header->meta.maxDataLength) {
		case MaxDataLength::DataLength_FF:
			header->row[h--] = 0;
			[[fallthrough]];
		case MaxDataLength::DataLength_FFFF:
			header->row[h--] = 0;
			[[fallthrough]];
		case MaxDataLength::DataLength_FFFFFF:
			header->row[h--] = 0;
			break;
		case MaxDataLength::DataLength_07:
			header->dataLength = header->meta.dataLength;

			h = -1;  // 後続のheader->detaLength格納処理をスキップさせる
			break;
	}
	while (h >= 0) header->row[h--] = buffer[b++];

	if (resource) {
		switch (resource->type) {
			case Corelnk:
			case String:
				resource->data.bytesValue.pointer = (void*)&buffer[b];
				break;
			case Opaque:
				resource->data.bytesValue.pointer = (void*)&buffer[b];
				resource->data.bytesValue.length  = header->dataLength;
				break;
			default:
				revcpy(resource->data.row, &buffer[b], header->dataLength);
				memset(&resource->data.row[header->dataLength], 0, TLVDATA_LENGTH - header->dataLength);
				break;
		}
	}

	return b + header->dataLength;
}

size_t Resource::getDataLength() {
	switch (type) {
		case Time:
			// Same Integer
		case Integer:
		case Unsigned:
			return 4;
			// Integerは1, 2, 4, 8byteのいずれかとあるが、SORACOMでは1, 2, 8byte長を許容していない
			// if (*((uint32_t *)&row[4]) != 0x00000000) return 8;
			// if (*((uint16_t *)&row[2]) != 0x0000) return 4;
			// if (*((uint8_t *)&row[1]) != 0x00) return 2;
			// return 1;
		case Float:
			// SORACOMでは4byteを許容していない（）要調査
			return 8;
		case Boolean:
			return 1;
		case Objlnk:
			return 4;
		case Corelnk:
			// Same String
		case String:
			if (!data.bytesValue.pointer) return 0;
			return strlen((const char*)data.bytesValue.pointer);
		case Opaque:
			if (!data.bytesValue.pointer) return 0;
			return data.bytesValue.length;
		case none:
			return 0;
	}
	return 0;	 // Error
}

void Resource::read() { callback(Read, &data); }

void Resource::update() { callback(Write, &data); }

void Resource::execute(const uint8_t* args, int length) {
	TLVData data = this->data;
	callback(Execute, &data);
}
