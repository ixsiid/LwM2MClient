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
	this->addResource(0, Operations::Read, TypeString);
	this->addResource(1, Operations::Read, TypeString);	    // Model Number
	this->addResource(2, Operations::Read, TypeString);	    // Serial Number
	this->addResource(3, Operations::Read, TypeString);	    // Firmware Version
	this->addResource(4, Operations::Execute, TypeNone);	    // Reboot
	this->addResource(5, Operations::Execute, TypeNone);	    // Factory Reset
	this->addResource(6, Operations::Read, TypeInteger);	    // Available Power Sources
	this->addResource(7, Operations::Read, TypeInteger);	    // Power Source Voltage
	this->addResource(8, Operations::Read, TypeInteger);	    // Power Source Current
	this->addResource(9, Operations::Read, TypeInteger);	    // Battery Level
	this->addResource(10, Operations::Read, TypeInteger);	    // Memory Free
	this->addResource(11, Operations::Read, TypeInteger);	    // Error Code
	this->addResource(12, Operations::Execute, TypeNone);	    // Reset Error Code
	this->addResource(13, Operations::ReadWrite, TypeTime);    // Current Time
	this->addResource(14, Operations::ReadWrite, TypeString);  // UTC Offset
	this->addResource(15, Operations::ReadWrite, TypeString);  // Timezone
	this->addResource(16, Operations::Read, TypeString);	    // Supported Binding and Modes
	this->addResource(17, Operations::Read, TypeString);	    // Device Type
	this->addResource(18, Operations::Read, TypeString);	    // Hardware Version
	this->addResource(19, Operations::Read, TypeString);	    // Software Version
	this->addResource(20, Operations::Read, TypeInteger);	    // Battery Status
	this->addResource(21, Operations::Read, TypeInteger);	    // Memory Total
	this->addResource(22, Operations::Read, TypeObjlnk);	    // ExtDevInfo
}

}  // namespace LwM2MObject
