#ifndef _AES_H
#define _AES_H

#include <sys/types.h>

namespace LwM2M {

class AES {
    public:
	static void aesEncrypt(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]);
	static void ctrEncrypt(uint8_t *key, uint8_t *nonce, uint16_t len, uint8_t *cipherText, uint8_t *plainText);
	static void cbcMAC(uint8_t *key, uint8_t *input, uint16_t len, uint8_t *mac);
};

}  // namespace LwM2M

#endif
