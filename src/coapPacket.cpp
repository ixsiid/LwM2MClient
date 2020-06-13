#include <stdlib.h>
#include <string.h>

#include "coapPacket.hpp"

using namespace LwM2M;

const uint8_t CoapOptCodeByte  = 13;
const uint8_t CoapOptCodeWord  = 14;
const uint16_t CoapOptByteBase = 13;
const uint16_t CoapOptWordBase = 269;
const uint8_t CoapOptionEnd    = 0xff;

CoapPacket::CoapPacket() {
	messageId = rand();
	timeout	= {3, 0};
}

CoapPacket* CoapPacket::Request(Coap::Type type, Coap::Code code, uint8_t* token) {
	this->option  = nullptr;
	this->payload = nullptr;

	buffer[0] = (CoapVersion << 6) | (type << 4) | TokenLength;
	buffer[1] = code;
	buffer[2] = (uint8_t)(messageId >> 8);
	buffer[3] = (uint8_t)(messageId >> 0);
	messageId++;
	if (token) {
		memcpy(&buffer[4], token, TokenLength);
	} else {
		for (int i = 0; i < TokenLength; i++) buffer[4 + i] = rand();
	}

	return this;
}

CoapPacket* CoapPacket::Response(Coap::Type type, Coap::Code code) {
	this->option  = nullptr;
	this->payload = nullptr;

	buffer[0] = (CoapVersion << 6) | (type << 4) | TokenLength;
	buffer[1] = code;

	return this;
}

CoapPacket* CoapPacket::Option(const Coap::Option* option) {
	this->option = option;
	return this;
}

CoapPacket* CoapPacket::Payload(const uint8_t* payload, size_t payloadLength) {
	this->payload		= payload;
	this->payloadLength = payloadLength;
	return this;
}

int CoapPacket::receive(IConnection* connection) {
	length	    = connection->receive(buffer, BufferLength, timeout);
	payload	    = nullptr;
	payloadLength = 0;
	return length;
}

void CoapPacket::getOption(Coap::Option* option) {
	size_t index = 4 + TokenLength;

	option->pathLen	= 0;
	option->queryLen	= 0;
	option->locationLen = 0;
	option->isObserve	= false;

	int currentOption = 0;

	payloadLength = 0;

	uint8_t* eq;

	while (index < length) {
		if (buffer[index] == 0xff) {
			payload	    = buffer + index + 1;
			payloadLength = length - index - 1;
			break;
		}

		int delta = (buffer[index] >> 4) & 0x0f;
		int l	= buffer[index] & 0x0f;
		index += 1;
		if (delta == CoapOptCodeByte) {
			delta = buffer[index] + CoapOptByteBase;
			index += 1;
		} else if (delta == CoapOptCodeWord) {
			delta = ((((int)buffer[index]) << 8) | buffer[index + 1]) + CoapOptWordBase;
			index += 2;
		}

		if (l == CoapOptCodeByte) {
			l = buffer[index] + CoapOptByteBase;
			index += 1;
		} else if (l == CoapOptCodeWord) {
			l = ((((int)buffer[index]) << 8) | buffer[index + 1]) + CoapOptWordBase;
			index += 2;
		}

		currentOption += delta;
		switch ((Coap::OptionType)currentOption) {
			case Coap::OptionType::Observe:
				option->isObserve = true;
				break;
			case Coap::OptionType::Location:
				memcpy(option->locations[option->locationLen], &buffer[index], l);
				option->locations[option->locationLen][l] = 0;
				option->locationLen++;
				break;
			case Coap::OptionType::Path:
				memcpy(option->paths[option->pathLen], &buffer[index], l);
				option->paths[option->pathLen][l] = 0;
				option->pathLen++;
				break;
			case Coap::OptionType::Query:
				eq = (uint8_t*)memchr(&buffer[index], '=', l - 1);

				size_t keyLen;
				size_t valueLen;
				if (eq) {
					keyLen   = eq - &buffer[index];
					valueLen = l - keyLen - 1;
				} else {
					keyLen   = l;
					valueLen = 0;
				}

				memcpy(option->queryKey[option->queryLen], &buffer[index], keyLen);
				option->queryKey[option->queryLen][keyLen] = '\0';
				memcpy(option->queryValue[option->queryLen], &buffer[index + keyLen + 1], valueLen);
				option->queryValue[option->queryLen][valueLen] = '\0';

				option->queryLen++;
				break;
			case Coap::OptionType::ContentFormat:
				break;
		}

		index += l;
	}
}

const uint8_t* CoapPacket::getPayload(size_t* size) {
	if (payloadLength > 0) {
		*size = payloadLength;
		return payload;
	}

	*size = 0;
	return payload;
}

void CoapPacket::setOptionLength(uint16_t delta, uint16_t dataLength) {
	int i = 1;
	if (delta < CoapOptByteBase) {
		buffer[length] = (uint8_t)(delta << 4);
	} else if (delta < CoapOptWordBase) {
		buffer[length]	    = (uint8_t)(CoapOptCodeByte << 4);
		buffer[length + i] = (uint8_t)(delta - CoapOptByteBase);
		i += 1;
	} else {
		buffer[length]		   = (uint8_t)(CoapOptCodeWord << 4);
		uint16_t w			   = delta - CoapOptWordBase;
		buffer[length + i + 0] = (uint8_t)(w >> 8);
		buffer[length + i + 1] = (uint8_t)(w);
		i += 2;
	}

	if (dataLength < CoapOptByteBase) {
		buffer[length] |= (uint8_t)(dataLength & 0x0f);
	} else if (dataLength < CoapOptWordBase) {
		buffer[length] |= (uint8_t)(CoapOptCodeByte & 0x0f);
		buffer[length + i] = (uint8_t)(dataLength - CoapOptByteBase);
		i += 1;
	} else {
		buffer[length] += CoapOptCodeWord;
		uint16_t w			   = dataLength - CoapOptWordBase;
		buffer[length + i + 0] = (uint8_t)(w >> 8);
		buffer[length + i + 1] = (uint8_t)(w >> 0);
		i += 2;
	}

	length += i;
}

bool CoapPacket::send(IConnection* connection) {
	length = 4 + TokenLength;

	if (option) {
		uint16_t currentOption = 0;
		for (int i = 0; i < option->pathLen; i++) {
			const char* path = option->paths[i];
			size_t len	  = strlen(path);
			setOptionLength(Coap::OptionType::Path - currentOption, len);
			memcpy(&buffer[length], path, len);
			length += len;
			currentOption = Coap::OptionType::Path;
		}

		if (option->isObserve) {
			setOptionLength(Coap::OptionType::Observe - currentOption, 2);
			buffer[length++] = (uint8_t)(option->observeCounter >> 8);
			buffer[length++] = (uint8_t)(option->observeCounter >> 0);
			currentOption	  = Coap::OptionType::Observe;
		}

		if (option->format != 0) {
			if (option->format < 255) {
				setOptionLength(Coap::OptionType::ContentFormat - currentOption, 1);
				buffer[length++] = (uint8_t)option->format;
			} else {
				setOptionLength(Coap::OptionType::ContentFormat - currentOption, 2);
				buffer[length++] = (uint8_t)(option->format >> 8);
				buffer[length++] = (uint8_t)(option->format >> 0);
			}
			currentOption = Coap::OptionType::ContentFormat;
		}

		for (int i = 0; i < option->queryLen; i++) {
			size_t keyLen	 = strlen(option->queryKey[i]);
			size_t valueLen = strlen(option->queryValue[i]);
			setOptionLength(Coap::OptionType::Query - currentOption, keyLen + valueLen + 1);
			memcpy(&buffer[length], option->queryKey[i], keyLen);
			length += keyLen;
			buffer[length++] = '=';
			memcpy(&buffer[length], option->queryValue[i], valueLen);
			length += valueLen;
			currentOption = Coap::OptionType::Query;
		}
	}

	if (payload) {
		buffer[length++] = 0xff;
		memcpy(&buffer[length], payload, payloadLength);
		length += payloadLength;
	}

	return connection->send(buffer, length);
}
