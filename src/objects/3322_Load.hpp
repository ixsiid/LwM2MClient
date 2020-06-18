#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
class LoadInstance : public LwM2MInstance {
    public:
	LoadInstance(int instanceId);
};
LoadInstance::LoadInstance(int instanceId) : LwM2MInstance(3322, instanceId) {
	// Object ID = 3322
	this->addResource(5700, Read, Type::Float);		 // Sensor Value
	this->addResource(5701, Read, Type::String);		 // Sensor Units
	this->addResource(5601, Read, Type::Float);		 // Min Measured Value
	this->addResource(5602, Read, Type::Float);		 // Max Measured Value
	this->addResource(5603, Read, Type::Float);		 // Min Range Value
	this->addResource(5604, Read, Type::Float);		 // Max Range Value
	this->addResource(5605, Execute, Type::none);	 // Reset Min and Max Measured Value
	this->addResource(5821, ReadWrite, Type::Float);	 // Current Calibration
	this->addResource(5750, ReadWrite, Type::String);	 // Application Type::
}

}  // namespace LwM2MObject
