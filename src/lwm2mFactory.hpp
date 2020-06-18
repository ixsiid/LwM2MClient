#ifndef __LWM2M_FACTORY_H
#define __LWM2M_FACTORY_H

#include <functional>

#include "list.hpp"
#include "lwm2mClient.hpp"
#include "lwm2mInstance.hpp"

namespace LwM2M {

static const int LWM2MFACTORY_MAX_NAME_LENGTH	= 32;
static const int LWM2MFACTORY_MAX_HOST_LENGTH	= 128;
static const int LWM2MFACTORY_MAX_IDENTITY_LENGTH = 32;
static const int LWM2MFACTORY_PSK_LENGTH		= 16;

class LwM2MFactory {
    public:
	LwM2MFactory(const char* name, uint16_t lifetime);
	LwM2MFactory& Ip(ip4_addr_t* ip);
	LwM2MFactory& Bootstrap(const char* host, int port);
	LwM2MFactory& SetSecurityPram(const char* identity, const uint8_t* psk);
	LwM2MFactory& AddInstance(LwM2MInstance* instance);
	LwM2MFactory& AddResource(int resourceId, ResourceCallback callback);
	LwM2MFactory& AddFixResource(int resourceId, char * data);
	LwM2MFactory& AddFixResource(int resourceId, uint8_t* data, size_t length);
	LwM2MFactory& AddFixResource(int resourceId, int64_t data);
	LwM2MClient* Regist(const char* host, int port);

    private:
	bool requireBootstraping;
	bool haveSecurityParam;
	bool isIpConfigured;

	uint16_t lifetime;

	List* instances;

	char name[LWM2MFACTORY_MAX_NAME_LENGTH];

	ip4_addr_t* ip;
	char host[LWM2MFACTORY_MAX_HOST_LENGTH];
	int port;

	char identity[LWM2MFACTORY_MAX_IDENTITY_LENGTH];
	uint8_t psk[LWM2MFACTORY_PSK_LENGTH];

	LwM2MInstance* currentInstance;
};

inline LwM2MFactory::AddFixResource(int resourceId, char * data) {
	AddFixResource(resourceId, (uint8_t *)data, strlen(data));
}

}  // namespace LwM2M

#endif  // __LWM2M_FACTORY_H
