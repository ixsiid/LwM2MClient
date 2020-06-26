#include "tlvMulti.hpp"

using namespace LwM2M;

#define LOG_LOCAL_LEVEL 3
#define TAG "TLV Multi"
#include "log.h"

TLVMulti::TLVMulti(uint16_t id, DataType type, void* value) : TLV(id, type, ResourceType::MultipleResouce) {
	_i("Multi initialize %p", value);
	_i("insted %p", (value ? (List *)value : new List()));
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
