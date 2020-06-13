#pragma once

#include <sys/types.h>

#include "IConnection.hpp"

namespace LwM2M {

namespace Coap {
typedef struct {
	uint16_t format = 0;

	bool isObserve		  = false;
	uint16_t observeCounter = 0;

	int pathLen = 0;
	char paths[3][16];

	int locationLen = 0;
	char locations[2][16];

	int queryLen = 0;
	char queryKey[4][16];
	char queryValue[4][64];
} Option;

enum OptionType {
	Observe	    = 6,
	Location	    = 8,
	Path		    = 11,
	ContentFormat = 12,
	Query	    = 15
};

// RFC7252 3. Message Format Type(T)参照
enum Type : uint8_t {
	Confirmable = 0,
	NonConfirmable,
	Acknowledgement,
	Reset
};

// RFC7252 12.1.1 Method Codes, 12.1.2 Response Codes参照
enum Code : uint8_t {
	Empty = 0,
	Get,
	Post,
	Put,
	Delete,
	OK = 64,		    // 2.00 OK
	Created,		    // 2.01 Created
	Deleted,		    // 2.02 Deleted
	Valid,		    // 2.03 Valid
	Changed,		    // 2.04 Changed
	Content,		    // 2.05 Content
	BadRequest = 128,  // 4.00 Bad Request
	Unauthorized,	    // 4.01 Unauthorized
	BadOption,	    // 4.02 Bad Option
	Forbidden,	    // 4.03 Forbidden
	NotFound,		    // 4.04 Not Found
	MethodNotAllowed,  // 4.05 Method Not Allowed
	NotAcceptable,	    // 4.06 Not Acceptable
};
}  // namespace Coap

class CoapPacket {
    public:
	/// https://tools.ietf.org/html/rfc7252#section-10.2.1
	CoapPacket();
	timeval timeout;

	/**
	 * 指定したType, Codeを持ったCoapパケットを作成します。
	 * 
	 * Responseとの違いは、パケットのMessageIDとして、内部変数が用いられ、
	 * このメソッドが呼ばれたタイミングでインクリメントされます。
	 * 使われるMessageIDは、Requestメソッドを呼び出す前に、
	 * getNextMessageIdを呼び出すことで取得できます。
	 * 
	 * @param CodeType type
	 * @param CoapCode code
	 * @param uint8_t* token = nullptr 送出するtokenを指定します。
	 *     nullptrの場合は乱数によって初期化されます。
	 *     通常、ObserveされたリソースのNotifyパケットを送信するに用います。
	 * 
	 * @return CoapPacket * (このメソッドはメソッドチェーンによって利用されます)
	 */
	CoapPacket* Request(Coap::Type type, Coap::Code code, uint8_t* token = nullptr);

	/**
	 * 指定したType, Codeを持ったCoapパケットを作成します。
	 * 
	 * Requestとの違いは、パケットのMessageIDとして、内部変数が用いられ、
	 * このメソッドが呼ばれたタイミングでインクリメントされます。
	 * 使われるMessageIDは、Requestメソッドを呼び出す前に、
	 * getNextMessageIdを呼び出すことで取得できます。
	 * 
	 * @param CodeType type
	 * @param CoapCode code
	 * @param uint8_t* token = nullptr
	 * 
	 * @return CoapPacket * (このメソッドはメソッドチェーンによって利用されます)
	 */
	CoapPacket* Response(Coap::Type type, Coap::Code code);

	/**
	 * Coapオプションを指定します。
	 */
	CoapPacket* Option(const Coap::Option* option);

	/**
	 * Coapペイロードを指定します。
	 */
	CoapPacket* Payload(const uint8_t* payload, size_t payloadLength);

	/**
	 * 作成されたCoapパケットをコネクションに送出します。
	 * 
	 * 通常このメソッドは、メソッドチェーンの最後に呼び出されます。
	 * 
	 * @param IConnection* connection
	 * 
	 * @return bool 送信が成功したが否か
	 */
	bool send(IConnection* connection);

	/**
	 * コネクションからCoapパケットを受信します。
	 * 
	 * この時点では、パケットのパースはされません。
	 * 
	 * @param IConnection* connection
	 * 
	 * @return int 受信したバイト数
	 *     = 0 の時、受信データが存在しない（タイムアウト）
	 *     < 0 の時、通信エラー（エラー内容はconnectionの実装に依存します）
	 */
	int receive(IConnection* connection);

	uint16_t getNextMessageId();
	uint16_t getMessageId();
	Coap::Type getType();
	Coap::Code getCode();
	void getOption(Coap::Option* option);

	/**
	 * Paylodのポインタを取得します。
	 * このメソッドを呼び出す前に、getOptionを呼び出す必要があります。
	 * 
	 * @param size_t* length Paylodのバイト長さを格納するポインタ
	 */
	const uint8_t* getPayload(size_t* length);
	const uint8_t* getToken();

	// void comp(uint8_t* src, int len);

	static const int CoapVersion	  = 1;
	static const size_t TokenLength = 8;

    private:
	static const size_t BufferLength = 4096;

	IConnection* connection;
	uint8_t buffer[BufferLength];
	size_t length;

	void setOptionLength(uint16_t delta, uint16_t length);

	uint16_t recvMessageId;
	const Coap::Option* option;
	const uint8_t* payload;
	size_t payloadLength;

	uint16_t messageId;
};

inline uint16_t CoapPacket::getNextMessageId() { return messageId; }
inline uint16_t CoapPacket::getMessageId() { return ((uint16_t)buffer[2] << 8) | buffer[3]; }
inline Coap::Type CoapPacket::getType() { return (Coap::Type)((buffer[0] >> 4) & 0x03); }
inline Coap::Code CoapPacket::getCode() { return (Coap::Code)buffer[1]; }
inline const uint8_t* CoapPacket::getToken() { return &buffer[4]; }
}  // namespace LwM2M
