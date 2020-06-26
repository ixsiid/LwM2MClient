/*
#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
// SecurityInstance は /0/0 しか存在しない
class ServerInstance : public LwM2MInstance {
    public:
	ServerInstance(bool sendOperation);
};

ServerInstance::ServerInstance(bool sendOperation) : LwM2MInstance(1, 1) {
	this->declareResource(0, Operations::Read, DataType::Integer, false, (void *)1);		 // Short Server ID
	this->declareResource(1, Operations::ReadWrite, DataType::Integer, false, (void *)120);	 // Lifetime
	this->declareResource(2, Operations::ReadWrite, DataType::Integer, false);	 // Default Minimum Period
	this->declareResource(3, Operations::ReadWrite, DataType::Integer, false);	 // Default Maximum Period
	this->declareResource(4, Operations::Execute, DataType::none, false);		 // Disable
	this->declareResource(5, Operations::ReadWrite, DataType::Integer, false);	 // Disable Timeout
	this->declareResource(6, Operations::ReadWrite, DataType::Boolean, false);	 // Notification Storing When Disabled or Offline
	this->declareResource(7, Operations::ReadWrite, DataType::String, false);	 // Binding
	this->declareResource(8, Operations::Execute, DataType::none, false);		 // Registration Update Trigger

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

const static Lwm2mResource serverResources[] = {
    // Object ID = 1
};
*/