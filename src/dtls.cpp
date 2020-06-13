#include "dtls.hpp"

#include "aes.hpp"
#include "endian.hpp"
#include "sha256.hpp"

using namespace LwM2M;

const static uint16_t DTLSVersion12	 = 0xfefd;
const static uint16_t CipherSuiteCCM8 = 0xc0a8;
const static uint8_t DTLSCompressNone = 0x00;

const static int DTLSHeaderLength	    = 13;
const static int HandshakeHeaderLength = 12;

const static uint8_t ContentTypeChangeCipherSpec = 20;
const static uint8_t ContentTypeAlert		  = 21;
const static uint8_t ContentTypeHandshake		  = 22;
const static uint8_t ContentTypeApplicationData  = 23;

const static uint8_t HandshakeTypeClientHello		= 1;
const static uint8_t HandshakeTypeClientKeyExchange = 16;
const static uint8_t HandshakeTypeFinished		= 20;

const static char *MasterSecretLabel   = "master secret";
const static char *KeyExpansionLabel   = "key expansion";
const static char *ClientFinishedLabel = "client finished";
const static char *ServerFinishedLabel = "server finished";

const timeval Dtls::defaultTimeout = {3, 0};
uint8_t Dtls::receiveBuffer[ReceiveBufferSize];

Dtls::Dtls(ip4_addr_t *ip, const char *host, int port) {
	serverEpoch	= 0;
	clientEpoch	= 0;
	serverSequence = 0;
	clientSequence = 0;
	memset(serverWriteKey, 0, 16);
	memset(clientWriteKey, 0, 16);
	memset(serverIV, 0, 4);
	memset(clientIV, 0, 4);
	clientEncrypt = 0;
	serverEncrypt = 0;

	verified = false;
	udp	    = new UDP(ip, host, port);
}

bool Dtls::handshaking(const char *identity, uint8_t *psk) {
	handshake.messageLen	= 0;
	handshake.serverSequence = 0;
	handshake.clientSequence = 0;
	memset(handshake.identity, 0, 64);
	memcpy(handshake.identity, identity, 22);
	memset(handshake.cookie, 0, 32);
	memset(handshake.session, 0, 64);
	memset(handshake.clientRandom, 0, 64);
	memset(handshake.serverRandom, 0, 64);
	memset(handshake.preMasterSecret, 0, 64);

	handshake.preMasterSecret[0] = 0;
	handshake.preMasterSecret[1] = 16;
	memset(&handshake.preMasterSecret[2], 0, 16);
	handshake.preMasterSecret[18] = 0;
	handshake.preMasterSecret[19] = 16;
	memcpy(&handshake.preMasterSecret[20], psk, 16);

	memset(handshake.masterSecret, 0, 64);
	memset(handshake.messages, 0, 1024);

	// Set client random
	memset(handshake.clientRandom, 0, 4);
	for (int i = 4; i < 32; i++) {
		handshake.clientRandom[i] = (uint8_t)random();
	}

	verified = getCookie();
	if (!verified) return false;

	verified = getSession();
	if (!verified) return false;

	sendClientKeyExchange();
	sendChangeCipherSpec();
	generateSecurityParams();
	if (sendFinished()) {
		verified = true;
		return true;
	} else {
		verified = false;
		return false;
	}
}

void Dtls::dtlsPrf(uint8_t *secret, int secretLen, uint8_t *label, int labelLen, uint8_t *seed, int seedLen, uint8_t *output, int outputLen) {
	int labelSeedLen = labelLen + seedLen;
	uint8_t labelSeed[labelSeedLen];
	memcpy(&labelSeed[0], label, labelLen);
	memcpy(&labelSeed[labelLen], seed, seedLen);

	uint8_t a[256];
	memcpy(a, labelSeed, labelSeedLen);
	int hashLen = labelSeedLen;
	uint8_t message[32 + labelSeedLen];
	int index = 0;

	while (index < outputLen) {
		hmacSha256(secret, secretLen, a, hashLen, a);
		hashLen = 32;
		memcpy(&message[0], a, 32);
		memcpy(&message[32], labelSeed, labelSeedLen);
		hmacSha256(secret, secretLen, message, 32 + labelSeedLen, message);
		if (index + 32 >= outputLen) {
			memcpy(&output[index], message, outputLen - index);
			index = outputLen;
			break;
		}
		memcpy(&output[index], message, 32);
		index += 32;
	}
}

void Dtls::generateSecurityParams() {
	unsigned char masterSecretSeed[64];
	memcpy(&masterSecretSeed[0], handshake.clientRandom, 32);
	memcpy(&masterSecretSeed[32], handshake.serverRandom, 32);
	dtlsPrf(
	    handshake.preMasterSecret, 36,
	    (uint8_t *)MasterSecretLabel, strlen(MasterSecretLabel),
	    masterSecretSeed, 64,
	    handshake.masterSecret, 48);

	unsigned char keyExpansionSeed[64];
	memcpy(&keyExpansionSeed[0], handshake.serverRandom, 32);
	memcpy(&keyExpansionSeed[32], handshake.clientRandom, 32);
	unsigned char keyBlock[40];
	dtlsPrf(
	    handshake.masterSecret, 48,
	    (uint8_t *)KeyExpansionLabel, strlen(KeyExpansionLabel),
	    keyExpansionSeed, 64,
	    keyBlock, 40);

	memcpy(clientWriteKey, &keyBlock[0], 16);
	memcpy(serverWriteKey, &keyBlock[16], 16);
	memcpy(clientIV, &keyBlock[32], 4);
	memcpy(serverIV, &keyBlock[36], 4);
}

void Dtls::generateMAC(uint8_t *key, uint8_t *nonce, uint8_t *epochSequence, uint8_t type, uint8_t *paddedData, uint16_t len, uint16_t paddedLen, uint8_t *mac) {
	// generate Aad
	uint8_t aad[13];
	memcpy(&aad[0], &epochSequence[0], 8);
	aad[8] = type;
	putUint16ToBytes(DTLSVersion12, &aad[9]);  // Version = DTLS1.2(0xfefd)
	putUint16ToBytes(len, &aad[11]);

	uint8_t flag = 90;  // (1 << 6) + (((dtlsAesCcmMACLength)-2)/2)<<3 + ((dtlsAesCCMLength) - 1)
	uint8_t blockForMAC[32 + paddedLen];
	memset(blockForMAC, 0, 32);
	blockForMAC[0] = flag;
	memcpy(&blockForMAC[1], nonce, 12);
	putUint24ToBytes((uint32_t)len, &blockForMAC[13]);
	putUint16ToBytes(13, &blockForMAC[16]);	 // aad length = 13
	memcpy(&blockForMAC[18], aad, 13);
	memcpy(&blockForMAC[32], paddedData, paddedLen);
	AES::cbcMAC(key, blockForMAC, paddedLen, mac);
}

void Dtls::dtlsEncrypt(const uint8_t *plain, uint16_t plainLen, uint8_t type, uint8_t *output) {
	uint8_t epochSequence[8];
	putUint16ToBytes(clientEpoch, &epochSequence[0]);
	putUint48ToBytes(clientSequence, &epochSequence[2]);

	uint8_t nonce[12];
	memcpy(&nonce[0], clientIV, 4);
	memcpy(&nonce[4], epochSequence, 8);

	uint8_t paddingLength = 0;
	if ((plainLen & 0x000F) > 0) {
		paddingLength = 16 - (plainLen & 0x000F);
	}

	uint16_t paddedLen = plainLen + paddingLength;

	uint8_t plainText[16 + paddedLen];  // 最初の16バイトは MAC, 以降に Data
	uint8_t *paddedData = plainText + 16;
	memcpy(paddedData, plain, plainLen);
	if (paddingLength > 0) {
		memset(&paddedData[plainLen], 0, paddingLength);
	}

	uint8_t *plainMAC = plainText;
	generateMAC(clientWriteKey, nonce, epochSequence, type, paddedData, plainLen, paddedLen, plainMAC);

	uint8_t cipherText[16 + paddedLen];
	AES::ctrEncrypt(clientWriteKey, nonce, paddedLen, plainText, cipherText);

	memcpy(&output[0], epochSequence, 8);
	memcpy(&output[8], &cipherText[16], plainLen);
	memcpy(&output[8 + plainLen], &cipherText[0], 8);
}

bool Dtls::dtlsDecrypt(const uint8_t *encrypted, uint16_t encryptedLen, uint8_t type, uint8_t *output) {
	uint8_t epochSequence[8];
	memcpy(epochSequence, &encrypted[0], 8);
	uint16_t encryptedDataLen = encryptedLen - 16;
	uint8_t encryptedData[encryptedDataLen];
	memcpy(encryptedData, &encrypted[8], encryptedDataLen);
	uint encryptedMAC[8];
	memcpy(encryptedMAC, &encrypted[encryptedLen - 8], 8);

	uint8_t paddingLength = 0;
	if ((encryptedDataLen & 0x000F) > 0) {
		paddingLength = 16 - (encryptedDataLen & 0x000F);
	}
	uint16_t paddedLen = encryptedDataLen + paddingLength;
	uint8_t paddedData[paddedLen];
	memcpy(&paddedData[0], encryptedData, encryptedDataLen);
	if (paddingLength > 0) {
		memset(&paddedData[encryptedDataLen], 0, paddingLength);
	}

	uint8_t nonce[12];
	memcpy(&nonce[0], serverIV, 4);
	memcpy(&nonce[4], epochSequence, 8);

	uint8_t cipherText[16 + encryptedDataLen + paddingLength];
	memcpy(&cipherText[0], &encryptedMAC[0], 8);
	memset(&cipherText[8], 0, 8);
	memcpy(&cipherText[16], &paddedData[0], encryptedDataLen + paddingLength);

	uint8_t plainText[16 + paddedLen];
	AES::ctrEncrypt(serverWriteKey, nonce, paddedLen, cipherText, plainText);

	uint8_t plainMAC[8];
	memcpy(&plainMAC[0], &plainText[0], 8);

	uint8_t calculatedMAC[16];
	uint8_t *plain = plainText + 16;

	if (paddingLength > 0) {
		memset(&plain[encryptedDataLen], 0, paddingLength);
	}
	generateMAC(serverWriteKey, nonce, epochSequence, type, plain, encryptedDataLen, paddedLen, calculatedMAC);

	// plainText の最初の8バイトは 複合化済みのMAC
	if (memcmp(plainText, calculatedMAC, 8) == 0) {
		memcpy(output, plain, encryptedDataLen);
		return 1;
	} else {
		return 0;
	}
}

int Dtls::generateDTLSHeader(uint8_t type, uint16_t len, uint8_t *packet) {
	uint16_t index	 = 0;
	packet[index++] = type;
	index += putUint16ToBytes(DTLSVersion12, &packet[index]);  // Version = DTLS1.2(0xfefd)
	index += putUint16ToBytes(clientEpoch, &packet[index]);
	index += putUint48ToBytes(clientSequence, &packet[index]);
	index += putUint16ToBytes(len, &packet[index]);
	return index;
}

int Dtls::generateHandshakeHeader(uint8_t type, uint16_t len, uint8_t *packet) {
	uint16_t index	 = 0;
	packet[index++] = type;
	index += putUint24ToBytes(len, &packet[index]);  // fragment length
	index += putUint16ToBytes(handshake.clientSequence, &packet[index]);
	index += putUint24ToBytes(0, &packet[index]);    // fragment offset = 0
	index += putUint24ToBytes(len, &packet[index]);  // fragment length
	return index;
}

bool Dtls::getCookie() {
	uint8_t packet[67];
	uint16_t index = 0;
	index += generateDTLSHeader(ContentTypeHandshake, 54, &packet[index]);
	index += generateHandshakeHeader(HandshakeTypeClientHello, 42, &packet[index]);

	// Client Hello Content
	index += putUint16ToBytes(DTLSVersion12, &packet[index]);  // Version = DTLS1.2(0xfefd)
	memcpy(&packet[index], handshake.clientRandom, 32);
	index += 32;
	packet[index++] = 0;								 // Session length = 0
	packet[index++] = 0;								 // Cookie length = 0
	index += putUint16ToBytes(2, &packet[index]);			 // CipherSuite length = 2
	index += putUint16ToBytes(CipherSuiteCCM8, &packet[index]);	 // CipherSuite is TLS_PSK_WITH_AES_128_CCM_8(0xc0a8)
	packet[index++] = 1;								 // Compress length = 1
	packet[index++] = DTLSCompressNone;					 // Compress is None(0)

	// Send Client Hello without cookie
	udp->send(packet, index);
	clientSequence++;
	handshake.clientSequence++;

	// Receive Hello Verify Request
	int readLen = udp->receive(receiveBuffer, ReceiveBufferSize, defaultTimeout);
	if (readLen == 0) {	 // Timeout
		// タイムアウトしたらverifiedを解除
		verified = false;
		return false;
	} else {
		// Parse Hello Verify Request
		memcpy(handshake.cookie, &receiveBuffer[DTLSHeaderLength + HandshakeHeaderLength + 3], 32);
		return true;
	}
}

bool Dtls::getSession() {
	uint8_t packet[99];
	uint16_t index = 0;

	index += generateDTLSHeader(ContentTypeHandshake, 86, &packet[index]);
	index += generateHandshakeHeader(HandshakeTypeClientHello, 74, &packet[index]);

	// Client Hello Content
	index += putUint16ToBytes(DTLSVersion12, &packet[index]);  // Version = DTLS1.2(0xfefd)
	memcpy(&packet[index], handshake.clientRandom, 32);
	index += 32;
	packet[index++] = 0;   // Session length = 0
	packet[index++] = 32;  // Cookie length = 32
	memcpy(&packet[index], handshake.cookie, 32);
	index += 32;
	index += putUint16ToBytes(2, &packet[index]);			 // CipherSuite length = 2
	index += putUint16ToBytes(CipherSuiteCCM8, &packet[index]);	 // CipherSuite is TLS_PSK_WITH_AES_128_CCM_8(0xc0a8)
	packet[index++] = 1;								 // Compress length = 1
	packet[index++] = DTLSCompressNone;					 // Compress is None(0)

	// Send Client Hello with cookie
	udp->send(packet, index);
	memcpy(&handshake.messages[handshake.messageLen], &packet[DTLSHeaderLength], 86);
	handshake.messageLen += 86;
	clientSequence++;
	handshake.clientSequence++;

	// Receive Server Hello, Server Hello Done
	int readLen = udp->receive(receiveBuffer, ReceiveBufferSize, defaultTimeout);

	if (readLen == 0) return false;  // Timeout

	// Parse Server Hello
	memcpy(handshake.serverRandom, &receiveBuffer[27], 32);
	memcpy(handshake.session, &receiveBuffer[60], 32);
	uint16_t packetLenServerHello = getUint16FromBytes(&receiveBuffer[11]);
	memcpy(&handshake.messages[handshake.messageLen], &receiveBuffer[13], packetLenServerHello);
	handshake.messageLen += packetLenServerHello;

	// Parse Server Hello Done
	uint16_t packetLenServerHelloDone = getUint16FromBytes(&receiveBuffer[13 + packetLenServerHello + 11]);
	memcpy(&handshake.messages[handshake.messageLen], &receiveBuffer[13 + packetLenServerHello + 13], packetLenServerHelloDone);
	handshake.messageLen += packetLenServerHelloDone;
	return true;
}

bool Dtls::sendClientKeyExchange() {
	uint8_t packet[49];
	uint16_t index = 0;

	index += generateDTLSHeader(ContentTypeHandshake, 36, &packet[index]);
	index += generateHandshakeHeader(HandshakeTypeClientKeyExchange, 24, &packet[index]);

	// Client Key Exchange Content
	index += putUint16ToBytes(22, &packet[index]);  // Identity length = 22
	memcpy(&packet[index], handshake.identity, 22);
	index += 22;

	// Send Client Key Exchange
	udp->send(packet, index);
	memcpy(&handshake.messages[handshake.messageLen], &packet[13], 36);
	handshake.messageLen += 36;
	clientSequence++;
	handshake.clientSequence++;

	return true;
}

bool Dtls::sendChangeCipherSpec() {
	uint8_t packet[14];
	uint16_t index = 0;

	index += generateDTLSHeader(ContentTypeChangeCipherSpec, 1, &packet[index]);

	// Change Cipher Spec Content
	packet[index++] = 1;  // Change CipherSpec Message = 1

	// Send Change Cipher Spec
	udp->send(packet, index);
	clientEpoch++;
	clientSequence = 0;
	clientEncrypt	= 1;

	return true;
}

bool Dtls::sendFinished() {
	uint8_t packet[53];
	uint16_t index = 0;
	index += generateDTLSHeader(ContentTypeHandshake, 40, &packet[index]);

	uint8_t plain[24];
	uint16_t plainIndex = 0;
	plainIndex += generateHandshakeHeader(HandshakeTypeFinished, 12, &plain[plainIndex]);

	// Finished Content
	uint8_t messageHash[32];
	sha256(handshake.messages, handshake.messageLen, messageHash);
	dtlsPrf(
	    handshake.masterSecret, 48,
	    (uint8_t *)ClientFinishedLabel, strlen(ClientFinishedLabel),
	    messageHash, 32,
	    &plain[12], 12);
	plainIndex += 12;

	// encrypt Finished
	dtlsEncrypt(plain, 24, 22, &packet[index]);
	index += plainIndex + 16;

	// Send encrypted Finished
	udp->send(packet, index);
	memcpy(&handshake.messages[handshake.messageLen], &plain[0], 24);
	handshake.messageLen += 24;
	clientSequence++;
	handshake.clientSequence++;

	// Receive Change Cipher Spec, encrypted Finished
	int readLen = udp->receive(receiveBuffer, ReceiveBufferSize, defaultTimeout);
	if (readLen == 0) {	 // Timeout
		return false;
	}

	// Parse Change Cipher Spec
	uint16_t packetLenChangeCipherSpec = getUint16FromBytes(&receiveBuffer[11]);
	serverEncrypt				   = 1;

	// Parse encrypted Finished
	uint16_t packetLenServerVerified = getUint16FromBytes(&receiveBuffer[13 + packetLenChangeCipherSpec + 11]);
	uint8_t encryptedVerified[packetLenServerVerified];
	memcpy(&encryptedVerified[0], &receiveBuffer[13 + packetLenChangeCipherSpec + 13], packetLenServerVerified);
	uint8_t verifiedPacket[packetLenServerVerified - 16];

	// Verify Data
	if (dtlsDecrypt(&encryptedVerified[0], packetLenServerVerified, 22, &verifiedPacket[0])) {
		uint8_t serverVerify[12];
		uint8_t serverMessageHash[32];
		sha256(handshake.messages, handshake.messageLen, serverMessageHash);
		dtlsPrf(
		    handshake.masterSecret, 48,
		    (uint8_t *)ServerFinishedLabel, strlen(ServerFinishedLabel),
		    serverMessageHash, 32,
		    &serverVerify[0], 12);
		if (memcmp(&verifiedPacket[12], &serverVerify[0], 12) == 0) {
			// VERIFY OK
			return true;
		} else {
			// VERIFY NG
			return false;
		}
	} else {
		// DECRYPT NG
		return false;
	}
}

bool Dtls::send(const uint8_t *data, uint16_t len) {
	uint16_t encryptedLen = len + 16;
	uint16_t packetLen	= encryptedLen + DTLSHeaderLength;
	uint8_t packet[packetLen];
	uint16_t index = 0;
	index += generateDTLSHeader(ContentTypeApplicationData, encryptedLen, &packet[index]);
	dtlsEncrypt(data, len, 23, &packet[index]);
	udp->send(packet, encryptedLen + DTLSHeaderLength);
	clientSequence++;
	return true;
}

int Dtls::receive(uint8_t *output, int length, timeval timeout) {
	int readLen = udp->receive(receiveBuffer, ReceiveBufferSize, timeout);
	if (readLen == 0) {
		// timeout
		return 0;
	}

	if (dtlsDecrypt(&receiveBuffer[13], readLen - 13, 23, output)) {
		int length = readLen - 13 - 16;
		if (length < 0) verified = false;
		return length;
	} else {
		verified = false;
		return -1;
	}
}
