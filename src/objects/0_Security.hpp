#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
// SecurityInstance は /0/0 しか存在しない
class SecurityInstance : public LwM2MInstance {
    public:
	SecurityInstance();
};

SecurityInstance::SecurityInstance() : LwM2MInstance(0, 1) {
	this->addResource(0, Operations::None, Type::String);
	this->addResource(1, Operations::None, Type::Boolean);
	this->addResource(2, Operations::None, Type::Integer);
	this->addResource(3, Operations::Write, Type::Opaque);	 // identity (デバイスID)
	this->addResource(4, Operations::None, Type::Opaque);
	this->addResource(5, Operations::Write, Type::Opaque);	 // psk
	this->addResource(6, Operations::None, Type::Integer);
	this->addResource(7, Operations::None, Type::Opaque);
	this->addResource(8, Operations::None, Type::Opaque);
	this->addResource(9, Operations::None, Type::String);
	this->addResource(10, Operations::None, Type::Integer);
	this->addResource(11, Operations::None, Type::Integer);
	this->addResource(12, Operations::None, Type::Integer);
}

}  // namespace LwM2MObject
