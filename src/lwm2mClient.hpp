#ifndef _LWM2M_H
#define _LWM2M_H

#include <sys/types.h>

#include "coapPacket.hpp"
#include "dtls.hpp"
#include "list.hpp"
#include "udp.hpp"
#include "lwm2mResource.hpp"

namespace LwM2M {

typedef struct {
	int counter;
	uint8_t token[8];
	uint16_t messageId;
} Observe;

class LwM2MClient {
    public:
	LwM2MClient(const char *endpoint, uint16_t lifetime, List *instances, ip4_addr_t *ip, const char *dstHost, int port);
	bool Bootstrap(const char *bootstrapHost, int bootstrapPort);
	bool Bootstrap();
	void SkipBootstrap(const char *identity, uint8_t *psk);
	bool CheckEvent();
	bool Notify(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);
	TLVData *getTLVData(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);

    private:
	bool Register();
	bool Update();

	void CoapProcess(IConnection *connection, std::function<void()> timeoutCallback = nullptr);

	void ReadResource(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);
	void WriteResource(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);
	void ExecuteResource(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);

	void DeleteRequest();
	void BootstrapFinishRequest();

	ip4_addr_t *ip;
	CoapPacket *coap;

	IConnection *bootstrapUdp;
	Dtls *dtls;
	char identity[32];
	uint8_t psk[16];

	uint8_t endpoint[64];
	uint64_t updatedTimestamp;
	uint64_t updatePeriod;
	char location[16];

	bool registered;
	bool bootstraped;

	List *instances;
	List *observes;

	Coap::Option registerOption;
	char registerInstances[256];
	size_t registerInstancesLength;

	static const size_t ReadOperationBufferSize = 256;
	static uint8_t readOperationBuffer[];
	static long convertObserveId(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);
};

}  // namespace LwM2M

#endif
