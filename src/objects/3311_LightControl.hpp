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
	this->declareResource(5850, ReadWrite, DataType::Boolean, false, (void*)false);	 // On/Off
	this->declareResource(5851, ReadWrite, DataType::Integer, false);			 // Dimmer
	this->declareResource(5852, ReadWrite, DataType::Integer, false);			 // On time
	this->declareResource(5805, Read, DataType::Float, false);					 // Cumulative active power
	this->declareResource(5820, Read, DataType::Float, false);					 // Power factor
	this->declareResource(5706, ReadWrite, DataType::String, false);				 // Colour
	this->declareResource(5701, Read, DataType::String, false);					 // Sensor Units
	this->declareResource(5750, ReadWrite, DataType::String, false);				 // Application Type
}

}  // namespace LwM2MObject
