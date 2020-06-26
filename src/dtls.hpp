#ifndef _DTLS_H
#define _DTLS_H

#include <sys/types.h>

#include "IConnection.hpp"
#include "udp.hpp"

namespace LwM2M {

typedef struct {
	uint16_t messageLen;
	uint16_t serverSequence;
	uint16_t clientSequence;
	uint8_t identity[64];
	uint8_t cookie[32];
	uint8_t session[64];
	uint8_t clientRandom[64];
	uint8_t serverRandom[64];
	uint8_t preMasterSecret[64];
	uint8_t masterSecret[64];
	uint8_t messages[1024];
} DtlsHandshake;

class Dtls : public IConnection {
    public:
	Dtls(ip4_addr_t *ip, const char *host, int port);
	bool handshaking(const char *identity, const uint8_t *psk);
	bool isVerified();

	bool send(const uint8_t *data, uint16_t length) override;
	int receive(uint8_t *buffer, int length, timeval timeout) override;

    private:
	static const timeval defaultTimeout;
	static const size_t ReceiveBufferSize = 4096;
	static uint8_t receiveBuffer[];

	uint16_t serverEpoch;
	uint16_t clientEpoch;
	uint64_t serverSequence;
	uint64_t clientSequence;
	uint8_t serverWriteKey[16];
	uint8_t clientWriteKey[16];
	uint8_t serverIV[4];
	uint8_t clientIV[4];
	bool clientEncrypt;
	bool serverEncrypt;

	UDP *udp;
	bool verified;

	DtlsHandshake handshake;

	bool getCookie();
	bool getSession();
	int generateDTLSHeader(uint8_t type, uint16_t len, uint8_t *packet);
	int generateHandshakeHeader(uint8_t type, uint16_t len, uint8_t *packet);
	void generateSecurityParams();
	bool sendChangeCipherSpec();
	bool sendClientKeyExchange();
	bool sendFinished();

	void dtlsEncrypt(const uint8_t *plain, uint16_t plainLen, uint8_t type, uint8_t *output);
	bool dtlsDecrypt(const uint8_t *encrypted, uint16_t encryptedLen, uint8_t type, uint8_t *output);
	static void dtlsPrf(uint8_t *secret, int secretLen, uint8_t *label, int labelLen, uint8_t *seed, int seedLen, uint8_t *output, int outputLen);
	static void generateMAC(uint8_t *key, uint8_t *nonce, uint8_t *epochSequence, uint8_t type, uint8_t *paddedData, uint16_t len, uint16_t paddedLen, uint8_t *mac);
};

inline bool Dtls::isVerified() {
	return verified;
}

}  // namespace LwM2M

#endif
