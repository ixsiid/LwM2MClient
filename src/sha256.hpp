#ifndef _SHA256_H
#define _SHA256_H

#include <sys/types.h>

namespace LwM2M
{
class SHA256
{
public:
    void sha256(const uint8_t *input, size_t len, const uint8_t *output);
    void hmacSha256(const uint8_t *secret, int secretLen, const uint8_t *message, int messageLen, const uint8_t *output);
private:
    void sha256Init();
    void sha256Transform(const uint32_t *data);
    void sha256End(const uint8_t digest[32]);
    void sha256Update(const uint8_t *data, size_t len);
};
} // namespace LwM2M

#endif
