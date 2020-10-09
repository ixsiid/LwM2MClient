#pragma once

#include "lwm2mInstance.hpp"

namespace LwM2MObject {
using namespace LwM2M;

enum Error {
	NoError			  = 0,
	LowBattery		  = 1,
	ExternalPowerOff	  = 2,
	GPSFailure		  = 3,
	LowSignal			  = 4,
	OutOfMemory		  = 5,
	SMSFailure		  = 6,
	NoIP				  = 7,
	PeripheralMalfunction = 8,
};

enum PowerSource {
	DC			 = 0,
	InternalBattery = 1,
	ExternalBattery = 2,
	PoE			 = 4,  // Power over Ethernet,
	USB			 = 5,
	AC			 = 6,
	Solar		 = 7,  //Solar The same Resource Instance ID MUST be used to associate a given Power Source(Resource ID : 6) with its Present Voltage(Resource ID : 7) and its Present Current(Resource ID : 8)
};

class DeviceInstance : public LwM2MInstance {
    public:
	DeviceInstance();
	void addError(Error error);
	void clearError();

    private:
	List* errors;	// Integer
	static TLVData noError;
};

DeviceInstance::DeviceInstance() : LwM2MInstance(3, 0) {
	errors = new List();  // Integer
	errors->add(0, (void*)Error::NoError);

	// Object ID = 3
	this->declareResource(0, Operations::Read, DataType::String, false);               // Manufacture
	this->declareResource(1, Operations::Read, DataType::String, false);			   // Model Number
	this->declareResource(2, Operations::Read, DataType::String, false);			   // Serial Number
	this->declareResource(3, Operations::Read, DataType::String, false);			   // Firmware Version
	this->declareResource(4, Operations::Execute, DataType::none, false);			   // Reboot
	this->declareResource(5, Operations::Execute, DataType::none, false);			   // Factory Reset
	this->declareResource(6, Operations::Read, DataType::Integer, true);			   // Available Power Sources
	this->declareResource(7, Operations::Read, DataType::Integer, true);			   // Power Source Voltage
	this->declareResource(8, Operations::Read, DataType::Integer, true);			   // Power Source Current
	this->declareResource(9, Operations::Read, DataType::Integer, false);			   // Battery Level
	this->declareResource(10, Operations::Read, DataType::Integer, false);		   // Memory Free
	this->declareResource(11, Operations::Read, DataType::Integer8, true, errors);	   // Error Code
	this->declareResource(12, Operations::Execute, DataType::none, false);		   // Reset Error Code
	this->declareResource(13, Operations::ReadWrite, DataType::Time, false);		   // Current Time
	this->declareResource(14, Operations::ReadWrite, DataType::String, false);		   // UTC Offset
	this->declareResource(15, Operations::ReadWrite, DataType::String, false);		   // Timezone
	this->declareResource(16, Operations::Read, DataType::String, false, (void*)"U");  // Supported Binding and Modes
	this->declareResource(17, Operations::Read, DataType::String, false);			   // Device DataType::
	this->declareResource(18, Operations::Read, DataType::String, false);			   // Hardware Version
	this->declareResource(19, Operations::Read, DataType::String, false);			   // Software Version
	this->declareResource(20, Operations::Read, DataType::Integer, false);		   // Battery Status
	this->declareResource(21, Operations::Read, DataType::Integer, false);		   // Memory Total
	this->declareResource(22, Operations::Read, DataType::Objlnk, false);			   // ExtDevInfo
}

void DeviceInstance::addError(Error error) {
	if (error == Error::NoError) return;

	errors->remove(0);
	errors->add(error, (void*)error);
}

void DeviceInstance::clearError() {
	errors->clear();
	errors->add(0, (void*)Error::NoError);
}

}  // namespace LwM2MObject
