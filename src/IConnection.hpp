#pragma once

#include <sys/types.h>

namespace LwM2M {
class IConnection {
    public:
	virtual bool send(const uint8_t *data, uint16_t length);
	virtual int receive(uint8_t *buffer, int length, timeval timeout);
};
}  // namespace LwM2M
