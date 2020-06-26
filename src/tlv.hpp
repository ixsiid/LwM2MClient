#pragma once

#include <sys/types.h>

#include <functional>

namespace LwM2M {

enum Operations {
	None			  = 0,
	Read			  = 1,
	Write		  = 2,
	ReadWrite		  = 3,
	Execute		  = 4,
	ReadExecute	  = 5,
	WriteExecute	  = 6,
	ReadWriteExecute = 7
};

enum DataType {
	String,
	Integer,
	Unsigned,
	Float,
	Boolean,
	Opaque,
	Time,
	Objlnk,
	Corelnk,
	none,
	Integer8
};

enum ResourceType {
	ObjectInstance	 = 0,
	ResouceInstance = 1,
	MultipleResouce = 2,
	Resouce		 = 3
};

enum MaxIdLength {
	IdLength_FF   = 0,
	IdLength_FFFF = 1,
};

enum MaxDataLength {
	DataLength_07	   = 0,
	DataLength_FF	   = 1,
	DataLength_FFFF   = 2,
	DataLength_FFFFFF = 3,
};

typedef struct __attribute__((packed)) {
	uint16_t objectId;
	uint16_t instanceId;
} link_t;

typedef struct {
	size_t length;
	void* pointer;
} pointer_t;

typedef union /*__attribute__((packed))*/ {
	uint8_t raw[8];
	int64_t int64Value;
	int32_t int32Value;
	int16_t int16Value;
	int8_t int8Value;
	float floatValue;
	double doubleValue;
	link_t linkValue;
	pointer_t bytesValue;
	char* textValue;
} TLVData;

typedef struct __attribute__((packed)) {
	size_t dataLength : 3;
	MaxDataLength maxDataLength : 2;
	MaxIdLength maxIdLength : 1;
	ResourceType resourceType : 2;
} TLVMeta;

typedef union {
	uint8_t raw[7];
	struct __attribute__((packed)) {
		size_t dataLength;
		uint16_t id;
		TLVMeta meta;
	};
} TLVHeader;

#include "coapPacket.hpp"

typedef std::function<bool(Operations operation, TLVData* data)> TLVCallback;

class TLV {
    public:
	TLV(uint16_t id, DataType type, ResourceType resourceType);
	virtual ~TLV();

	virtual void add(const void* value) = 0;
	void setCallback(TLVCallback callback);
	virtual bool hasValue() = 0;
	size_t Serialize(uint8_t* buffer);
	bool Deserialize(const uint8_t* buffer, size_t* length = nullptr);
	bool execute(const uint8_t* buffer = nullptr, size_t length = 0);

    protected:
	DataType type;
	TLVCallback callback;

	TLVData data;

    private:
	TLVHeader header;
	static size_t parse(TLV* tlv, const uint8_t* buffer);

	size_t getDataLength();
};

inline void TLV::setCallback(TLVCallback callback) { this->callback = callback; }
inline bool TLV::Deserialize(const uint8_t* buffer, size_t* length) {
	size_t l = parse(this, buffer);
	if (length) *length = l;
	return l > 0;
}

}  // namespace LwM2M
