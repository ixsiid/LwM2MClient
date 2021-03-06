#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
// SecurityInstance は /0/0 しか存在しない
class SecurityInstance : public LwM2MInstance {
    public:
	SecurityInstance();

	SecurityInstance *getIdentity(char *buffer);
	SecurityInstance *getPsk(char *buffer);

    private:
	char identity[32];
	char psk[16];
};

inline SecurityInstance *SecurityInstance::getIdentity(char *buffer) {
	strncpy(buffer, identity, 32);
	return this;
}
inline SecurityInstance *SecurityInstance::getPsk(char *buffer) {
	memcpy(buffer, psk, 16);
	return this;
}

SecurityInstance::SecurityInstance() : LwM2MInstance(0, 1) {
	this->declareResource(0, Operations::None, DataType::String, false);
	this->declareResource(1, Operations::None, DataType::Boolean, false);
	this->declareResource(2, Operations::None, DataType::Integer, false);
	this->declareResource(3, Operations::Write, DataType::Opaque, false);	 // identity (デバイスID)
	this->declareResource(4, Operations::None, DataType::Opaque, false);
	this->declareResource(5, Operations::Write, DataType::Opaque, false);	 // psk
	this->declareResource(6, Operations::None, DataType::Integer, false);
	this->declareResource(7, Operations::None, DataType::Opaque, false);
	this->declareResource(8, Operations::None, DataType::Opaque, false);
	this->declareResource(9, Operations::None, DataType::String, false);
	this->declareResource(10, Operations::None, DataType::Integer, false);
	this->declareResource(11, Operations::None, DataType::Integer, false);
	this->declareResource(12, Operations::None, DataType::Integer, false);

	this->addResource(3, [&](Operations operation, TLVData *tlv) {
		strncpy(this->identity, (const char *)tlv->bytesValue.pointer, 32);
		return true;
	});
	this->addResource(5, [&](Operations operation, TLVData *tlv) {
		strncpy(this->psk, (const char *)tlv->bytesValue.pointer, 16);
		return true;
	});
}

}  // namespace LwM2MObject
