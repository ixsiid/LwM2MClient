#ifndef __LWM2M_RESOURCE_H
#define __LWM2M_RESOURCE_H

#include <functional>

#include <sys/types.h>

namespace LwM2M {

typedef enum {
	None			  = 0,
	Read			  = 1,
	Write		  = 2,
	ReadWrite		  = 3,
	Execute		  = 4,
	ReadExecute	  = 5,
	WriteExecute	  = 6,
	ReadWriteExecute = 7
} Operations;

typedef enum {
	TypeString = 0,
	TypeInteger,
	TypeFloat,
	TypeBoolean,
	TypeOpaque,
	TypeTime,
	TypeObjlnk,
	TypeNone
} ResourceType;

typedef struct {
	int dataLength;
	union {
		int8_t byteValue;
		int16_t wordValue;
		int32_t shortValue;
		int64_t longValue;
		double floatValue;
		uint32_t objectInstanceLinkValue;
		uint8_t* bytesValue;
	};
} TLVData;

class Resource {
    public:
	Resource(int id, Operations operation, ResourceType type);
	int Serialize(uint8_t* buffer);
	int parse(const uint8_t* buffer);

	void registCallback(std::function<void(Operations operation, TLVData* tlv, int dataLength)> callback);
	void read();
	void update();
	void execute(const uint8_t* args, int length);

	bool isRead();
	bool isWrite();
	bool isExecute();

	static uint16_t getTLVId(const uint8_t* buffer);
	static int calculateParseLength(const uint8_t* buffer);

    private:
	ResourceType type;
	Operations operation;
	int typeOfId;	// not use
	int id;

	std::function<void(Operations operation, TLVData* tlv, int dataLength)> callback;

	TLVData data;

	static void defaultOperation(Operations operation, TLVData* tlv, int dataLength);
};

}  // namespace LwM2M

#endif  // __LWM2M_RESOURCE_H