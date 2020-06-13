#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
class LightControlInstance : public LwM2MInstance {
    public:
	LightControlInstance(int instanceId);
};
LightControlInstance::LightControlInstance(int instanceId) : LwM2MInstance(3311, instanceId) {
	// Object ID = 3311
	this->addResource(5850, ReadWrite, TypeBoolean);	// On/Off
	this->addResource(5851, ReadWrite, TypeInteger);	// Dimmer
	this->addResource(5852, ReadWrite, TypeInteger);	// On time
	this->addResource(5805, Read, TypeFloat);		// Cumulative active power
	this->addResource(5820, Read, TypeFloat);		// Power factor
	this->addResource(5706, ReadWrite, TypeString);	// Colour
	this->addResource(5701, Read, TypeString);		// Sensor Units
	this->addResource(5750, ReadWrite, TypeString);	// Application Type
}

}  // namespace LwM2MObject
