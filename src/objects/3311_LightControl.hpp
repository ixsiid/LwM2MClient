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
	this->addResource(5850, ReadWrite, Type::Boolean);  // On/Off
	this->addResource(5851, ReadWrite, Type::Integer);  // Dimmer
	this->addResource(5852, ReadWrite, Type::Integer);  // On time
	this->addResource(5805, Read, Type::Float);		  // Cumulative active power
	this->addResource(5820, Read, Type::Float);		  // Power factor
	this->addResource(5706, ReadWrite, Type::String);	  // Colour
	this->addResource(5701, Read, Type::String);		  // Sensor Units
//	this->addResource(5750, ReadWrite, Type::String);	  // Application Type
}

}  // namespace LwM2MObject
