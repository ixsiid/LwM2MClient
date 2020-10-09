#pragma once

#include <sys/types.h>
#include <lwip/inet.h>

class SoracomDevice {
    public:
	SoracomDevice(const char* host, const char* deviceKey, const char* deviceSecret);
	~SoracomDevice();

	const char* getDeviceKey();
	const uint8_t* getDeviceSecret();
	const ip4_addr_t* getHost();
	const char* getHostStr();

    private:
	static char base64Code(char a);

	const char* deviceKey;
	uint8_t* deviceSecret;

	char host_addr[16];
	ip4_addr_t host;
};

inline const char* SoracomDevice::getDeviceKey() { return this->deviceKey; }
inline const uint8_t* SoracomDevice::getDeviceSecret() { return this->deviceSecret; }
inline const ip4_addr_t* SoracomDevice::getHost() { return &this->host; }
inline const char* SoracomDevice::getHostStr() { return this->host_addr; }

