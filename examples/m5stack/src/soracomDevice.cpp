#include "soracomDevice.hpp"

#include <lwip/dns.h>
#include <stdlib.h>

#include <cstring>

struct dns_callback_result_t {
	ip4_addr_t* addr;
	bool founded;
};

SoracomDevice::SoracomDevice(const char* host, const char* deviceKey, const char* deviceSecret) {
	this->deviceKey = deviceKey;

	// Decode device secret;
	size_t len	 = strlen(deviceSecret);
	uint8_t* secret = (uint8_t*)malloc(len * 3 / 4);

	int j = 0;
	for (int i = 0; i < len; i += 4) {
		char a0 = base64Code(deviceSecret[i + 0]);
		char a1 = base64Code(deviceSecret[i + 1]);
		char a2 = base64Code(deviceSecret[i + 2]);
		char a3 = base64Code(deviceSecret[i + 3]);

		secret[j++] = (a0 << 2) | (a1 >> 4);
		secret[j++] = (a1 << 4) | (a2 >> 2);
		secret[j++] = (a2 << 6) | (a3 >> 0);
	}
	this->deviceSecret = secret;

	// Lookup Host
	dns_callback_result_t result;
	result.addr = &this->host;
	result.founded = false;
	ip_addr_t localhost;
	dns_gethostbyname(
	    host, &localhost, [](const char* name, const ip_addr_t* ipaddr, void* arg) {
		    dns_callback_result_t* t = (dns_callback_result_t*)arg;

		    *t->addr	= ipaddr->u_addr.ip4;
		    t->founded = true;
	    },
	    &result);
	while (!result.founded) continue;

	sprintf(host_addr, "%i.%i.%i.%i",
		   ip4_addr1(&this->host),
		   ip4_addr2(&this->host),
		   ip4_addr3(&this->host),
		   ip4_addr4(&this->host));
}

SoracomDevice::~SoracomDevice() {
	free(this->deviceSecret);
}

char SoracomDevice::base64Code(char a) {
	if (a >= 'A' && a <= 'Z') return a - 0x41 + 0b000000;
	if (a >= 'a' && a <= 'z') return a - 0x61 + 0b011010;
	if (a >= '0' && a <= '9') return a - 0x30 + 0b110100;
	if (a == '+') return 0b111110;
	if (a == '/') return 0b111111;
	if (a == '=') return 0b000000;
	return 0xff;
}