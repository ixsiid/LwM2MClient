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
	this->addResource(5700, Read, TypeFloat);	    // Sensor Value
	this->addResource(5701, Read, TypeString);	    // Sensor Units
	this->addResource(5601, Read, TypeFloat);	    // Min Measured Value
	this->addResource(5602, Read, TypeFloat);	    // Max Measured Value
	this->addResource(5603, Read, TypeFloat);	    // Min Range Value
	this->addResource(5604, Read, TypeFloat);	    // Max Range Value
	this->addResource(5605, Execute, TypeNone);	    // Reset Min and Max Measured Value
	this->addResource(5821, ReadWrite, TypeFloat);   // Current Calibration
	this->addResource(5750, ReadWrite, TypeString);  // Application Type
}

}  // namespace LwM2MObject
