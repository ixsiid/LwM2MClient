#include "lwm2mInstance.hpp"

#include "tlvMulti.hpp"
#include "tlvSingle.hpp"

#define LOG_LOCAL_LEVEL 3
#define TAG "LwM2M Instance"
#include "log.h"

using namespace LwM2M;

LwM2MInstance::LwM2MInstance(int objectId, int instanceId) {
	this->objectId	  = objectId;
	this->instanceId = instanceId;
	resources		  = new List();
}

void LwM2MInstance::addResource(int resourceId, void* value) {
	Resource_t* x = (Resource_t*)resources->find(resourceId);
	if (!x) {
		_e("Undefined resource %d", resourceId);
		return;
	}

	if (!x->resource) {
		if (x->multiple) {
			List* values = new List();
			values->add(resourceId, value);
			x->resource = new TLVMulti(resourceId, x->type, values);
		} else {
			x->resource = new TLVSingle(resourceId, x->type, value);
		}
	} else {
		x->resource->add(value);
	}
}

void LwM2MInstance::addResource(int resourceId, TLVCallback callback) {
	Resource_t* x = (Resource_t*)resources->find(resourceId);
	if (!x) {
		_e("Undefined resource %d", resourceId);
		return;
	}

	if (!x->resource) {
		x->resource = x->multiple ? (TLV*)new TLVMulti(resourceId, x->type, callback) : (TLV*)new TLVSingle(resourceId, x->type, callback);
	} else {
		x->resource->setCallback(callback);
	}
}

void LwM2MInstance::declareResource(int resourceId, Operations operation, DataType type, bool multiple, void* value) {
	Resource_t* res = new Resource_t();
	res->multiple	 = multiple;
	res->resourceId = resourceId;
	res->type		 = type;
	res->operation	 = operation;

	if (value) {
		res->mandatory = true;

		if (multiple) {
			List* values = new List();
			values->add(resourceId, value);
			res->resource = new TLVMulti(resourceId, type, values);
		} else {
			res->resource = new TLVSingle(resourceId, type, value);
		}
	} else {
		res->mandatory = false;
	}

	Resource_t* old = (Resource_t*)resources->add(resourceId, res);
	if (old) {
		delete old->resource;
		delete old;
	}
}

long LwM2MInstance::getId(uint16_t objectId, uint16_t instanceId) {
	long id = objectId;
	id <<= 16;
	return id += instanceId;
}

Coap::Code LwM2MInstance::readResource(Resource_t* target, uint8_t* buffer, size_t* writed_length) {
	*writed_length = 0;
	if (!target) return Coap::Code::NotFound;
	if (!(target->operation & Operations::Read)) return Coap::Code::NotAcceptable;

	TLV* resource = target->resource;
	if (!resource) return Coap::Code::NotFound;
	if (!resource->hasValue()) return Coap::Code::NotFound;

	//	target->read(); hasValueの時に更新しちゃう
	*writed_length = resource->Serialize(buffer);
	return Coap::Code::Content;
}

Coap::Code LwM2MInstance::readAll(uint8_t* buffer, size_t* length) {
	*length = 0;
	resources->all([&](long id, void* data) {
		size_t l;
		readResource((Resource_t*)data, buffer + *length, &l);
		*length += l;
	});
	return *length > 0 ? Coap::Code::Content : Coap::Code::NotFound;
}

Coap::Code LwM2MInstance::read(int resourceId, uint8_t* buffer, size_t* writed_length) {
	return readResource((Resource_t*)resources->find(resourceId), buffer, writed_length);
}

Coap::Code LwM2MInstance::writeResource(Resource_t* target, const uint8_t* buffer, size_t* read_length, bool bootstrapping) {
	if (!target) return Coap::Code::NotFound;
	if (!bootstrapping && !(target->operation & Operations::Write)) return Coap::Code::NotAcceptable;

	TLV* resource = target->resource;
	if (!resource) return Coap::Code::NotFound;
	return resource->Deserialize(buffer, read_length) ? Coap::Code::Changed : Coap::Code::MethodNotAllowed;
}

Coap::Code LwM2MInstance::write(uint16_t resourceId, const uint8_t* buffer, bool bootstrapping) {
	Resource_t* target = (Resource_t*)resources->find(resourceId);
	return writeResource(target, buffer, nullptr, bootstrapping);
}

Coap::Code LwM2MInstance::writeAll(const uint8_t* buffer, size_t length, bool bootstrapping) {
	size_t index = 0;
	resources->all([&](long id, void* data) {
		size_t l;
		writeResource((Resource_t*)data, buffer + index, &l, bootstrapping);
		index += l;
		if (index > length) abort();
	});
	return index > 0 ? Coap::Code::Content : Coap::Code::NotFound;
}

Coap::Code LwM2MInstance::execute(int resourceId, const uint8_t* buffer, int length) {
	Resource_t* target = (Resource_t*)resources->find(resourceId);
	if (!target) return Coap::Code::NotFound;

	if (!(target->operation & Operations::Execute)) return Coap::Code::NotAcceptable;

	TLV* resource = target->resource;
	if (!resource) return Coap::Code::NotFound;
	return resource->execute() ? Coap::Code::OK : Coap::Code::BadRequest;
}
