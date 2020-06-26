#include "tlv.hpp"

#include <cstring>

#include "list.hpp"

#define LOG_LOCAL_LEVEL 3
#define TAG "Resource"
#include "log.h"

using namespace LwM2M;

void revcpy(uint8_t* output, const uint8_t* input, size_t length) {
	size_t i = 0;
	while (length > 0) output[i++] = input[--length];
}

TLV::TLV(uint16_t id, DataType type, ResourceType resourceType) {
	this->type = type;

	this->callback = nullptr;

	this->header.id			 = id;
	this->header.meta.maxIdLength	 = id <= 0xff ? MaxIdLength::IdLength_FF : MaxIdLength::IdLength_FFFF;
	this->header.meta.resourceType = resourceType;
}

TLV::~TLV() {}

size_t TLV::Serialize(uint8_t* buffer) {
	header.dataLength	   = getDataLength();
	header.meta.dataLength = 0;

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
	buffer[b++] = header.raw[h--];
	// id
	if (header.raw[h] != 0)
		buffer[b++] = header.raw[h--];
	else
		h--;
	buffer[b++] = header.raw[h--];

	// data length
	while (header.raw[h] == 0) h--;
	while (h >= 0) buffer[b++] = header.raw[h--];

	size_t len = header.dataLength ? header.dataLength : header.meta.dataLength;

	if (header.meta.resourceType == ResourceType::MultipleResouce) {
		_i("Serialize Multi");
		TLVHeader _header = header;
		TLVData _data	   = data;

		size_t t = 0;

		header.dataLength		= 0;
		header.meta.resourceType = ResourceType::ResouceInstance;
		header.id				= 0;
		header.meta.maxIdLength	= MaxIdLength::IdLength_FF;
		_i("%p", data.bytesValue.pointer);
		((List*)data.bytesValue.pointer)->all([&](long id, void* _data) {
			// header.meta.maxIdLength = header.id <= 0xff ? MaxIdLength::IdLength_FF : MaxIdLength::IdLength_FFFF;
			memset(&data, 0, sizeof(TLVData));
			switch (type) {
				case Boolean:
				case Integer8:
					data.int8Value = (int8_t)(int32_t)_data;
					break;
				case Objlnk:
					// data.linkValue = (link_t)_data;
					// break;
				case Float:
					// data.floatValue = (float)_data;
					// break;
				case Integer:
				case Unsigned:
				case Time:
					data.int32Value = (int32_t)_data;
					break;
				case Corelnk:
				case String:
					data.textValue = (char*)_data;
					break;
				case Opaque:
					data.bytesValue = *(pointer_t*)_data;
					break;
				case none:
					break;
			}
			t += Serialize(buffer + b + t);
			header.id++;
		});

		data	  = _data;
		header = _header;
	} else {
		_i("Serialize Single");
		switch (type) {
			case Corelnk:
			case String:
				memcpy(buffer + b, data.textValue, len);
				break;
			case Opaque:
				memcpy(buffer + b, data.bytesValue.pointer, len);
				break;
			default:
				revcpy(buffer + b, data.raw, len);
				break;
		}
	}

	char n[256];
	for (int i = 0; i < b + len; i++) {
		sprintf(n + i * 3, "%2x ", buffer[i]);
	}
	_i("buffer: %s", n);

	return b + len;
}

size_t TLV::getDataLength() {
	bool multi		= header.meta.resourceType == ResourceType::MultipleResouce;
	size_t count		= 1;
	size_t headerLength = 0;

	List* list = (List*)data.bytesValue.pointer;
	if (multi) {
		if (!data.bytesValue.pointer) return 0;
		if ((count = list->count()) == 0) return 0;
		headerLength = 2;
	}

	switch (type) {
		case Integer8:
			return (1 + headerLength) * count;
		case Time:  // Same Integer
		case Integer:
		case Unsigned:
			return (4 + headerLength) * count;
			// Integerは1, 2, 4, 8byteのいずれかとあるが、SORACOMでは1, 2, 8byte長を許容していない?
			// if (*((uint32_t *)&raw[4]) != 0x00000000) return 8;
			// if (*((uint16_t *)&raw[2]) != 0x0000) return 4;
			// if (*((uint8_t *)&raw[1]) != 0x00) return 2;
			// return 1;
		case Float:
			// SORACOMでは4byteを許容していない（）要調査
			return (4 + headerLength) * count;
		case Boolean:
			return (1 + headerLength) * count;
		case Objlnk:
			return (4 + headerLength) * count;
		case Corelnk:	// Same String
		case String:
			if (multi) {
				size_t l = 0;
				list->all([&](long id, void* data) {
					size_t _l = strlen((const char*)data);
					l += headerLength + _l;
					if (_l > 7) l++;
					if (_l > 0xff) l++;
					if (_l > 0xffff) l++;
				});
				return l;
			} else {
				return strlen(data.textValue);
			}
			break;
		case Opaque:
			if (multi) {
				size_t l = 0;
				list->all([&](long id, void* data) {
					pointer_t* p = (pointer_t*)data;
					size_t _l	   = p->length;
					l += headerLength + _l;
					if (_l > 7) l++;
					if (_l > 0xff) l++;
					if (_l > 0xffff) l++;
				});
				return l;
			} else {
				if (!data.bytesValue.pointer) return 0;
				return data.bytesValue.length;
			}
		case none:
			return 0;
	}

	return 0;	 // Error
}

bool TLV::execute(const uint8_t* buffer, size_t length) {
	if (!callback) return false;
	data.bytesValue.length  = length;
	data.bytesValue.pointer = (void*)buffer;
	callback(Operations::Execute, &data);
	return true;
}

size_t TLV::parse(TLV* tlv, const uint8_t* buffer) {
	TLVHeader _header;
	TLVHeader* header = tlv ? &tlv->header : &_header;

	size_t b = 0;
	int h    = 6;

	header->raw[h--] = buffer[b++];

	// id
	header->raw[h--] = header->meta.maxIdLength == MaxIdLength::IdLength_FF ? 0 : buffer[b++];
	header->raw[h--] = buffer[b++];

	// datalength

	switch (header->meta.maxDataLength) {
		case MaxDataLength::DataLength_FF:
			header->raw[h--] = 0;
			[[fallthrough]];
		case MaxDataLength::DataLength_FFFF:
			header->raw[h--] = 0;
			[[fallthrough]];
		case MaxDataLength::DataLength_FFFFFF:
			header->raw[h--] = 0;
			break;
		case MaxDataLength::DataLength_07:
			header->dataLength = header->meta.dataLength;

			h = -1;  // 後続のheader->detaLength格納処理をスキップさせる
			break;
	}
	while (h >= 0) header->raw[h--] = buffer[b++];

	size_t multiDataLength = 0;
	if (header->meta.resourceType == ResourceType::MultipleResouce) {
		// Writeでマルチデータは発生しない気がする
		abort();
	} else {
		if (tlv) {
			switch (tlv->type) {
				case Corelnk:
				case String:
					tlv->data.bytesValue.pointer = (void*)&buffer[b];
					break;
				case Opaque:
					tlv->data.bytesValue.pointer = (void*)&buffer[b];
					tlv->data.bytesValue.length  = header->dataLength;
					break;
				default:
					revcpy(tlv->data.raw, &buffer[b], header->dataLength);
					memset(&tlv->data.raw[header->dataLength], 0, sizeof(TLVData) - header->dataLength);
					break;
			}
		}
	}

	return b + header->dataLength + multiDataLength;
}
