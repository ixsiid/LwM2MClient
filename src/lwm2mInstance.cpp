#include "lwm2mInstance.hpp"

#define TAG "LwM2M Instance"
#include "log.h"

using namespace LwM2M;

LwM2MInstance::LwM2MInstance(int objectId, int instanceId) {
	this->objectId	  = objectId;
	this->instanceId = instanceId;
	resources		  = new List();
}

void LwM2MInstance::addResource(int resourceId, Operations operation, Type type) {
	resources->add(resourceId, new Resource(resourceId, operation, type));
}

void LwM2MInstance::registCallback(int resourceId, ResourceCallback callback) {
	Resource* target = (Resource*)resources->find(resourceId);
	if (target) target->registCallback(callback);
}

void LwM2MInstance::setFixResource(int resourceId, TLVData *data) {
	Resource* target = (Resource*)resources->find(resourceId);
	if (target) target->setData(data);
}

long LwM2MInstance::getId(uint16_t objectId, uint16_t instanceId) {
	long id = objectId;
	id <<= 16;
	return id += instanceId;
}

long LwM2MInstance::getId() { return getId(objectId, instanceId); }

int LwM2MInstance::readAll(uint8_t* buffer) {
	int length = 0;
	resources->all([&](long id, void* data) {
		Resource* target = (Resource*)data;
		if (target->isRead()) {
			target->read();
			length += target->Serialize(&buffer[length]);
		}
	});
	return length;
}

int LwM2MInstance::read(int resourceId, uint8_t* buffer) {
	Resource* target = (Resource*)resources->find(resourceId);
	if (!target) return -1;

	if (target->isRead()) {
		target->read();
		return target->Serialize(buffer);
	}

	return -1;
}

Coap::Code LwM2MInstance::write(uint16_t resourceId, const uint8_t* buffer, bool bootstrapping) {
	Resource* target = (Resource*)resources->find(resourceId);
	if (!target) return Coap::Code::NotFound;

	if (bootstrapping || target->isWrite()) {
		target->Deserialize(buffer);
		target->update();
		return Coap::Code::Changed;
	}

	return Coap::Code::MethodNotAllowed;
}

Coap::Code LwM2MInstance::writeAll(const uint8_t* buffer, int length, bool bootstrapping) {
	Coap::Code result = Coap::Code::NotFound;
	int index		   = 0;
	while (index < length) {
		int id	  = Resource::getId(&buffer[index]);
		Resource* target = (Resource*)resources->find(id);
		if (target) {
			result = Coap::Code::MethodNotAllowed;

			if (bootstrapping || target->isWrite()) {
				index += target->Deserialize(&buffer[index]);
				target->update();
			} else {
				index += Resource::calculateParse(&buffer[index]);
			}
		}
	}
	return index > 0 ? Coap::Code::Changed : result;
}

Coap::Code LwM2MInstance::execute(int resourceId, const uint8_t* buffer, int length) {
	Resource* target = (Resource*)resources->find(resourceId);
	if (!target) return Coap::Code::NotFound;

	if (target->isExecute()) {
		target->execute(buffer, length);
		return Coap::Code::Changed;
	}

	return Coap::Code::MethodNotAllowed;
}
