#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
class LoadInstance : public LwM2MInstance {
    public:
	LoadInstance(int instanceId, float initialValue = 0.0f);
};
LoadInstance::LoadInstance(int instanceId, float initialValue) : LwM2MInstance(3322, instanceId) {
	float f = initialValue;
	// Object ID = 3322
	this->declareResource(5700, Read, DataType::Float, false, (void *)*(int32_t *)&f);  // Sensor Value
	this->declareResource(5701, Read, DataType::String, false);					    // Sensor Units
	this->declareResource(5601, Read, DataType::Float, false);					    // Min Measured Value
	this->declareResource(5602, Read, DataType::Float, false);					    // Max Measured Value
	this->declareResource(5603, Read, DataType::Float, false);					    // Min Range Value
	this->declareResource(5604, Read, DataType::Float, false);					    // Max Range Value
	this->declareResource(5605, Execute, DataType::none, false);				    // Reset Min and Max Measured Value
	this->declareResource(5821, ReadWrite, DataType::Float, false);				    // Current Calibration
	this->declareResource(5750, ReadWrite, DataType::String, false);				    // Application DataType::
}

}  // namespace LwM2MObject
