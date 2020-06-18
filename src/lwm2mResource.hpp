#ifndef __LWM2M_RESOURCE_H
#define __LWM2M_RESOURCE_H

#include <sys/types.h>

#include <cstring>
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

enum TLVType {
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

enum Type {
	String,
	Integer,
	Unsigned,
	Float,
	Boolean,
	Opaque,
	Time,
	Objlnk,
	Corelnk,
	none
};

typedef struct __attribute__((packed)) {
	uint16_t objectId;
	uint16_t instanceId;
} link_t;

typedef struct {
	size_t length;
	void* pointer;
} pointer_t;

/// CoAPパケット長がMTU長以下にならなければならない
/// そのため1024程度確保して、切り詰めるのが一般的
/// 閉域網など特殊なネットワークではMTUが64byteなど極端に短い例や
/// ロングフレーム対応ネットワークでは64kbyteなど極端に長い例もある
static const size_t TLVDATA_LENGTH = 8;

typedef union /*__attribute__((packed))*/ {
	uint8_t raw[TLVDATA_LENGTH];
	int64_t int64Value;
	int32_t int32Value;
	int16_t int16Value;
	int8_t int8Value;
	float floatValue;
	double doubleValue;
	link_t linkValue;
	pointer_t bytesValue;
} TLVData;

typedef struct __attribute__((packed)) {
	size_t dataLength : 3;
	MaxDataLength maxDataLength : 2;
	MaxIdLength maxIdLength : 1;
	TLVType typeOfId : 2;
} TLVMeta;

typedef union {
	uint8_t raw[7];
	struct __attribute__((packed)) {
		size_t dataLength;
		uint16_t id;
		TLVMeta meta;
	};
} TLVHeader;

typedef std::function<void(Operations operation, TLVData* tlv)> ResourceCallback;

class Resource {
    public:
	Resource(uint16_t id, Operations operation, Type type);
	int Serialize(uint8_t* buffer);
	size_t Deserialize(const uint8_t* buffer);

	void registCallback(ResourceCallback callback);
	void setData(TLVData* data);

	void read();
	void update();
	void execute(const uint8_t* args, int length);

	bool isRead();
	bool isWrite();
	bool isExecute();

	uint16_t getId();

	static size_t calculateParse(const uint8_t* buffer);
	static uint16_t getId(const uint8_t* buffer);

    private:
	Operations operation;

	TLVHeader header;
	TLVData data;
	Type type;

	size_t getDataLength();

	ResourceCallback callback;

	static size_t parse(Resource* resource, const uint8_t* buffer);

	static void nullOperation(Operations operation, TLVData* tlv);
};

inline void Resource::registCallback(ResourceCallback callback) { this->callback = callback; }
inline void Resource::setData(TLVData* data) { memcpy(&this->data, data, sizeof(TLVData)); }
inline size_t Resource::calculateParse(const uint8_t* buffer) { return Resource::parse(nullptr, buffer); }
inline uint16_t Resource::getId() { return header.id; }
inline uint16_t Resource::getId(const uint8_t* buffer) {
	TLVHeader header;
	header.raw[6] = buffer[0];
	uint16_t id   = buffer[1];
	if (header.meta.maxIdLength == MaxIdLength::IdLength_FFFF) {
		id <<= 8;
		id += buffer[2];
	}
	return id;
}

}  // namespace LwM2M

#endif  // __LWM2M_RESOURCE_H
