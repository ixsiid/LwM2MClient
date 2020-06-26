#include "tlvSingle.hpp"

using namespace LwM2M;

TLVSingle::TLVSingle(uint16_t id, DataType type, void* value) : TLV(id, type, ResourceType::Resouce) {
	_hasValue = false;
	add(value);
}

TLVSingle::TLVSingle(uint16_t id, DataType type, TLVCallback callback) : TLV(id, type, ResourceType::Resouce) {
	_hasValue = callback != nullptr;
	setCallback(callback);
}

TLVSingle::~TLVSingle() {}

void TLVSingle::add(const void* value) {
	if (!value) return;
	switch (type) {
		case DataType::Integer8:
		case DataType::Boolean:
			data.int8Value = (int8_t)(int32_t)value;
			break;
		case DataType::Float:
			// break;
		case DataType::Time:
		case DataType::Unsigned:
		case DataType::Integer:
			data.int32Value = (int32_t)value;
			break;
		case DataType::none:
			break;
		case DataType::Objlnk:
			data.linkValue = *(link_t*)value;
			break;
		case DataType::Corelnk:
			// Same String
			// break;
		case DataType::String:
			data.textValue = (char*)value;
			break;
		case DataType::Opaque:
			data.bytesValue = *(pointer_t*)value;
			break;
	}
	_hasValue = true;
}

bool TLVSingle::hasValue() {
	if (callback) {
		return callback(Operations::Read, &data);
	}
	return _hasValue;
}
