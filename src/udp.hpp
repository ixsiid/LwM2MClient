#ifndef __UDP_H
#define __UDP_H

#include <sys/types.h>
#include <lwip/sockets.h>

#include "IConnection.hpp"

namespace LwM2M {

class UDP : public IConnection {
    public:
	UDP(ip4_addr_t *my_ip, const char *host, int port);
	bool send(const uint8_t *buffer, uint16_t length) override;
	int receive(uint8_t *buffer, int length, timeval timeout) override;

    private:
	struct sockaddr_in addr;
	int _socket;
};

}  // namespace LwM2M

#endif  // __UDP_H
