#include "udp.hpp"

#include <errno.h>
#include <esp_log.h>
#include <lwip/dns.h>
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sys.h>

#define TAG "LwM2M::UDP"
#include "log.h"

using namespace LwM2M;

UDP::UDP(ip4_addr_t *my_ip, const char *host, int port) {
	_socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (_socket < 0) {
		_e("Socket error");
		return;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family	 = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port		 = htons(port);
/*
	// Receiver initialize
	_socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (_socket < 0) {
		_e("")
		throw;
	}
*/
}

bool UDP::send(const uint8_t *buffer, uint16_t length) {
	int l = 0;
	while (length > l) {
		int s = sendto(_socket, &buffer[l], length, 0, (struct sockaddr *)&addr, sizeof(addr));
		if (s < 0) return false;
		l += s;
	}
	return true;
}

int UDP::receive(uint8_t *buffer, int length, timeval timeout) {
	setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeval));

	int recv_length = recvfrom(_socket, buffer, length, 0, nullptr, nullptr);
	if (recv_length < 0) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		return 0;
	}
	return recv_length;
}
