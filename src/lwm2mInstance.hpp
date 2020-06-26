#pragma once

#include <string.h>
#include <sys/types.h>

#include "coapPacket.hpp"
#include "list.hpp"
#include "tlv.hpp"

namespace LwM2M {

typedef struct {
	int resourceId;
	DataType type;
	bool mandatory;
	bool multiple;
	TLV* resource;
	Operations operation;
} Resource_t;

class LwM2MInstance {
    protected:
	LwM2MInstance(int objectId, int instanceId);
	void declareResource(int resourceId, Operations operation, DataType type, bool multiple, void* value = nullptr);

    private:
	List* resources;  // <Resource_t *>
	uint16_t objectId, instanceId;

	Coap::Code readResource(Resource_t* target, uint8_t* buffer, size_t* writed_length);
	Coap::Code writeResource(Resource_t* target, const uint8_t* buffer, size_t* read_length, bool bootstrapping);

    public:
	void addResource(int resourceId, TLVCallback callback);
	void addResource(int resourceId, void* value);

	Coap::Code read(int resourceId, uint8_t* buffer, size_t* writed_length);
	Coap::Code readAll(uint8_t* buffer, size_t * writed_length);
	Coap::Code write(uint16_t resourceId, const uint8_t* buffer, bool bootstrapping = false);
	Coap::Code writeAll(const uint8_t* buffer, size_t length, bool bootstrapping = false);
	Coap::Code execute(int resourceId, const uint8_t* buffer, int length);

	static long getId(uint16_t objectId, uint16_t instanceId);
	long getId();
};

inline long LwM2MInstance::getId() { return getId(objectId, instanceId); }

}  // namespace LwM2M

