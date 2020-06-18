#ifndef __LWM2M_INSTANCE_H
#define __LWM2M_INSTANCE_H

#include <string.h>

#include <sys/types.h>
#include <functional>

#include "endian.hpp"
#include "list.hpp"
#include "lwm2mResource.hpp"
#include "coapPacket.hpp"

namespace LwM2M {

class LwM2MInstance {
    protected:
	LwM2MInstance(int objectId, int instanceId);
	void addResource(int resourceId, Operations operation, Type type);

    private:
	List* resources;  // <TLV *>
	uint16_t objectId, instanceId;

    public:
	void registCallback(int resourceId, ResourceCallback callback);
	void setFixResource(int resourceId, TLVData *data);

	int read(int resourceId, uint8_t* buffer);
	int readAll(uint8_t* buffer);
	Coap::Code write(uint16_t resourceId, const uint8_t* buffer, bool bootstrapping = false);
	Coap::Code writeAll(const uint8_t* buffer, int length, bool bootstrapping = false);
	Coap::Code execute(int resourceId, const uint8_t* buffer, int length);

	static long getId(uint16_t objectId, uint16_t instanceId);
	long getId();
};

}  // namespace LwM2M

#endif  // __LWM2M_INSTANCE_H
