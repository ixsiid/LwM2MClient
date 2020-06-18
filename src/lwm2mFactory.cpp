#include "lwm2mFactory.hpp"

#include <esp_system.h>

#define TAG "LwM2M Factory"
#include "log.h"

using namespace LwM2M;

LwM2MFactory::LwM2MFactory(const char* name, uint16_t lifetime) {
	requireBootstraping = false;
	haveSecurityParam	= false;
	
	size_t free_heap = esp_get_free_heap_size();
	if (free_heap < 15000) {
		_w("Free heap memory size is %d, LwM2M Client used about 12kbyte heap memory.", free_heap);
	}

	this->lifetime = lifetime;

	strncpy(this->name, name, LWM2MFACTORY_MAX_NAME_LENGTH);

	instances = new List();
}

LwM2MFactory& LwM2MFactory::Ip(ip4_addr_t* ip) {
	isIpConfigured = true;

	this->ip = ip;

	return *this;
}

LwM2MFactory& LwM2MFactory::Bootstrap(const char* host, int port) {
	if (haveSecurityParam) return *this;
	requireBootstraping = true;

	strncpy(this->host, host, LWM2MFACTORY_MAX_HOST_LENGTH);
	this->port = port;

	return *this;
}

LwM2MFactory& LwM2MFactory::SetSecurityPram(const char* identity, const uint8_t* psk) {
	if (requireBootstraping) return *this;
	haveSecurityParam = true;

	strncpy(this->identity, identity, LWM2MFACTORY_MAX_IDENTITY_LENGTH);
	memcpy(this->psk, psk, LWM2MFACTORY_PSK_LENGTH);

	return *this;
}

LwM2MFactory& LwM2MFactory::AddInstance(LwM2MInstance* instance) {
	void* existInstance = instances->add(instance->getId(), instance);
	if (existInstance) {
		long id = instance->getId();
		_i("Exist instance /%u/%u", (uint16_t)id >> 16, (uint16_t)id & 0xffff);
		free(existInstance);
	}
	currentInstance = instance;

	return *this;
}

LwM2MFactory& LwM2MFactory::AddResource(int resourceId, ResourceCallback callback) {
	currentInstance->registCallback(resourceId, callback);
	return *this;
}

LwM2MFactory& LwM2MFactory::AddFixResource(int resourceId, uint8_t* data, size_t length) {
	TLVData d;
	d.bytesValue.pointer = data;
	d.bytesValue.length	 = length;
	currentInstance->setFixResource(resourceId, &d);
	return *this;
}

LwM2MFactory& LwM2MFactory::AddFixResource(int resourceId, int64_t data) {
	TLVData d;
	d.int64Value = data;
	currentInstance->setFixResource(resourceId, &d);
	return *this;
}

LwM2MClient* LwM2MFactory::Regist(const char* host, int port) {
	if (!isIpConfigured) return nullptr;

	_i("Start construct");
	LwM2MClient* lwm2m = new LwM2MClient(name, lifetime, instances, ip, host, port);
	if (requireBootstraping) {
		lwm2m->Bootstrap(this->host, this->port);
		while (!lwm2m->Bootstrap()) {
			_i("bootstrapping");
			continue;
		}
	} else if (haveSecurityParam) {
		lwm2m->SkipBootstrap(identity, psk);
	} else {
		return nullptr;
	}

	_i("start");
	return lwm2m;
}
