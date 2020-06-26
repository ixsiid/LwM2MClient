#include "tlvMulti.hpp"

using namespace LwM2M;

TLVMulti::TLVMulti(uint16_t id, DataType type, void* value) : TLV(id, type, ResourceType::MultipleResouce) {
	data.bytesValue.pointer = list = (value ? (List *)value : new List());
}

TLVMulti::TLVMulti(uint16_t id, DataType type, TLVCallback callback) : TLV(id, type, ResourceType::MultipleResouce) {
	setCallback(callback);
}

TLVMulti::~TLVMulti() {
	free(data.bytesValue.pointer);
}

void TLVMulti::clear() {
	list->clear();
}

void TLVMulti::set(const void* value) {
	clear();
	add(value);
}

void TLVMulti::add(const void* value) {
	if (!value) return;
	return;
}

bool TLVMulti::hasValue() {
	return list->count() > 0;
}
