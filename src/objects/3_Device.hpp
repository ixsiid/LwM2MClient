#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;
class DeviceInstance : public LwM2MInstance {
    public:
	DeviceInstance(int instanceId);
};
DeviceInstance::DeviceInstance(int instanceId) : LwM2MInstance(3, instanceId) {
	// Object ID = 3
	this->addResource(0, Operations::Read, Type::String);
	this->addResource(1, Operations::Read, Type::String);		 // Model Number
	this->addResource(2, Operations::Read, Type::String);		 // Serial Number
	this->addResource(3, Operations::Read, Type::String);		 // Firmware Version
	this->addResource(4, Operations::Execute, Type::none);		 // Reboot
	this->addResource(5, Operations::Execute, Type::none);		 // Factory Reset
	this->addResource(6, Operations::Read, Type::Integer);		 // Available Power Sources
	this->addResource(7, Operations::Read, Type::Integer);		 // Power Source Voltage
	this->addResource(8, Operations::Read, Type::Integer);		 // Power Source Current
	this->addResource(9, Operations::Read, Type::Integer);		 // Battery Level
	this->addResource(10, Operations::Read, Type::Integer);	 // Memory Free
	this->addResource(11, Operations::Read, Type::Integer);	 // Error Code
	this->addResource(12, Operations::Execute, Type::none);	 // Reset Error Code
	this->addResource(13, Operations::ReadWrite, Type::Time);	 // Current Time
	this->addResource(14, Operations::ReadWrite, Type::String);	 // UTC Offset
	this->addResource(15, Operations::ReadWrite, Type::String);	 // Timezone
	this->addResource(16, Operations::Read, Type::String);		 // Supported Binding and Modes
	this->addResource(17, Operations::Read, Type::String);		 // Device Type::
	this->addResource(18, Operations::Read, Type::String);		 // Hardware Version
	this->addResource(19, Operations::Read, Type::String);		 // Software Version
	this->addResource(20, Operations::Read, Type::Integer);	 // Battery Status
	this->addResource(21, Operations::Read, Type::Integer);	 // Memory Total
	this->addResource(22, Operations::Read, Type::Objlnk);		 // ExtDevInfo
}

}  // namespace LwM2MObject
