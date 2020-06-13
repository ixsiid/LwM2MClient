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
	this->addResource(0, Operations::None, TypeString);
	this->addResource(1, Operations::None, TypeBoolean);
	this->addResource(2, Operations::None, TypeInteger);
	this->addResource(3, Operations::Write, TypeOpaque); // identity (デバイスID)
	this->addResource(4, Operations::None, TypeOpaque);
	this->addResource(5, Operations::Write, TypeOpaque); // psk
	this->addResource(6, Operations::None, TypeInteger);
	this->addResource(7, Operations::None, TypeOpaque);
	this->addResource(8, Operations::None, TypeOpaque);
	this->addResource(9, Operations::None, TypeString);
	this->addResource(10, Operations::None, TypeInteger);
	this->addResource(11, Operations::None, TypeInteger);
	this->addResource(12, Operations::None, TypeInteger);
}

}  // namespace LwM2MObject
