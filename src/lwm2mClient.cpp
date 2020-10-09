#include "lwm2mClient.hpp"

#include "lwm2mInstance.hpp"
#include "objects/0_Security.hpp"

#define TAG "LwM2M Client"
#include "log.h"

using namespace LwM2M;

uint8_t LwM2MClient::readOperationBuffer[ReadOperationBufferSize];

LwM2MClient::LwM2MClient(const char *endpoint, uint16_t lifetime, List *instances, ip4_addr_t *ip, const char *dstHost, int port) {
	this->ip = ip;

	registered  = false;
	bootstraped = false;

	dtls = new Dtls(ip, dstHost, port);
	coap = new CoapPacket();

	this->instances = instances;
	this->observes	 = new List();

	updatePeriod = lifetime * 9 / 10 * 1000000;

	// Register用データ（通信不安定時など、Re-registerが発生すると再利用する）
	strcpy(registerOption.paths[0], "rd");
	registerOption.pathLen = 1;
	registerOption.format  = 0x28;
	strcpy(registerOption.queryKey[0], "LwM2M: ");
	strcpy(registerOption.queryValue[0], "1.0");
	strcpy(registerOption.queryKey[1], "ep");
	strcpy(registerOption.queryValue[1], endpoint);
	strcpy(registerOption.queryKey[2], "b");
	strcpy(registerOption.queryValue[2], "U");
	strcpy(registerOption.queryKey[3], "lt");
	itoa(lifetime, registerOption.queryValue[3], 10);
	registerOption.queryLen = 4;

	strcpy(registerInstances, "</>;rt=\"oma.lwm2m\";ct=11543");
	//                 length: 12345678901234567890123456789
	registerInstancesLength = 29;	 // strlen(registerInstances);

	instances->all([&](long id, void *data) {
		uint16_t objectId = id >> 16;
		if (objectId == 0) return;  // Security Instanceは登録時に送付しない

		registerInstancesLength += sprintf(registerInstances + registerInstancesLength, ",</%u/%u>", objectId, (uint16_t)(id & 0xffff));
	});

	dtlsHandshakeRetryCount = 0;
}

void LwM2MClient::SkipBootstrap(const char *identity, uint8_t *psk) {
	strcpy(this->identity, identity);
	memcpy(this->psk, psk, 16);

	bootstraped = true;
}

bool LwM2MClient::Bootstrap(const char *bootstrapHost, int bootstrapPort) {
	bootstrapUdp = new UDP(ip, bootstrapHost, bootstrapPort);

	LwM2MInstance *securityInstance = new LwM2MObject::SecurityInstance();
	instances->add(0, securityInstance);

	return Bootstrap();
}

bool LwM2MClient::Bootstrap() {
	Coap::Option options;
	strcpy(options.paths[0], "bs");
	options.pathLen = 1;
	strcpy(options.queryKey[0], "ep");
	strcpy(options.queryValue[0], registerOption.queryValue[1]);
	options.queryLen = 1;

	coap->Request(Coap::Type::Confirmable, Coap::Code::Post)
	    ->Option(&options)
	    ->send(bootstrapUdp);

	_i("LwM2M: Start bootstrap");

	while (!bootstraped) {
		_i("LwM2M: bootstrap while");
		CoapProcess(bootstrapUdp);
		return false;
	}

	((LwM2MObject::SecurityInstance *)instances->find(0))->getIdentity(identity)->getPsk((char *)psk);
	_i("%s, %s", identity, psk);

	_i("LwM2M: finish bootstrap");
	return true;
}

bool LwM2MClient::CheckEvent() {
	if (!dtls->isVerified()) {
		if (dtlsHandshakeRetryCount > 5) {
			_i("LwM2M: DTLS handshake retry over");
			vTaskDelay(120 * 1000 / portTICK_PERIOD_MS);
			return false;
		}
		dtlsHandshakeRetryCount++;

		_i("LwM2M: dtls not verified");

		if (dtls->handshaking(this->identity, this->psk)) {
			updatedTimestamp = 0;

			location[0]  = '\0';
			location[15] = '\0';  // strncpyで文字数ギリギリだと NULL終端されないため
			registered   = false;
		}
		return true;
	}

	if (!registered) {
		_i("Register");
		Register();
		return true;
	}

	if (esp_timer_get_time() - updatedTimestamp > updatePeriod) {
		_i("Update");
		Update();
		return true;
	}

	CoapProcess(dtls);

	return false;
}

void LwM2MClient::ReadResource(uint16_t objectId, uint16_t instanceId, uint16_t resourceId) {
	_i("READ /%u/%u/%u", objectId, instanceId, resourceId);

	if (instanceId == 0xffff) return;

	Coap::Option options;
	options.format = 0x2d16;

	Coap::Code result = Coap::Code::NotFound;
	size_t readLength = 0;

	LwM2MInstance *instance = (LwM2MInstance *)instances->find(LwM2MInstance::getId(objectId, instanceId));
	if (instance) {
		if (resourceId == 0xffff) {
			result = instance->readAll(readOperationBuffer, &readLength);
			if (readLength > 0) result = Coap::Code::Content;
		} else {
			result = instance->read(resourceId, readOperationBuffer, &readLength);
		}
	}

	char b[256];
	for (int i = 0; i < readLength; i++) {
		sprintf(b + i * 3, "%2x ", readOperationBuffer[i]);
	}
	_i("Payload: %p, %d", readOperationBuffer, readLength);
	_i("%s", b);

	coap->Response(Coap::Type::Acknowledgement, result)
	    ->Option(&options)
	    ->Payload(readOperationBuffer, readLength)
	    ->send(dtls);
}

void LwM2MClient::WriteResource(uint16_t objectId, uint16_t instanceId, uint16_t resourceId) {
	_i("WRITE /%u/%u/%u", objectId, instanceId, resourceId);

	Coap::Code result = Coap::Code::NotFound;

	LwM2MInstance *instance = (LwM2MInstance *)instances->find(LwM2MInstance::getId(objectId, instanceId));
	if (instance) {
		size_t size;
		const uint8_t *payload = coap->getPayload(&size);

		if (resourceId == 0xffff) {
			result = instance->writeAll(payload, size, !bootstraped);
		} else {
			result = instance->write(resourceId, payload, !bootstraped);
		}
	}

	coap->Response(Coap::Type::Acknowledgement, result)
	    ->send(dtls);
}

void LwM2MClient::ExecuteResource(uint16_t objectId, uint16_t instanceId, uint16_t resourceId) {
	_i("EXECUTE /%u/%u/%u", objectId, instanceId, resourceId);

	Coap::Code result = Coap::Code::NotFound;

	LwM2MInstance *instance = (LwM2MInstance *)instances->find(LwM2MInstance::getId(objectId, instanceId));
	if (instance) {
		size_t size;
		const uint8_t *payload = coap->getPayload(&size);

		_i("execute");

		result = instance->execute(resourceId, payload, size);
	}

	coap->Response(Coap::Type::Acknowledgement, result)
	    ->send(dtls);
}

void LwM2MClient::DeleteRequest() {
	if (!bootstraped) {
		coap->Response(Coap::Type::Acknowledgement, Coap::Code::Deleted)
		    ->send(bootstrapUdp);
	}
}

void LwM2MClient::BootstrapFinishRequest() {
	coap->Response(Coap::Type::Acknowledgement, Coap::Code::Changed)
	    ->send(bootstrapUdp);
	bootstraped = true;
}

bool LwM2MClient::Register() {
	coap->Request(Coap::Type::Confirmable, Coap::Code::Post)
	    ->Option(&registerOption)
	    ->Payload((const uint8_t *)registerInstances, registerInstancesLength)
	    ->send(dtls);

	CoapProcess(dtls, [&]() { registered = false; });

	_i("registered: %d", registered);

	return registered;
}

bool LwM2MClient::Update() {
	Coap::Option options;
	strcpy(options.paths[0], "rd");
	strcpy(options.paths[1], location);
	options.pathLen = 2;

	coap->Request(Coap::Type::Confirmable, Coap::Code::Post)
	    ->Option(&options)
	    ->send(dtls);

	CoapProcess(dtls, [&]() { registered = false; });

	return registered;
}

long LwM2MClient::convertObserveId(uint16_t objectId, uint16_t instanceId, uint16_t resourceId) {
	// 48bit を 32bitに圧縮
	// 先頭16bitに objectId のビット順序反転した結果
	// 後方16bitに resourceId
	// 中間 4bitに instanceId を足し算

	// objectIdのビット順序反転
	objectId = ((objectId & 0x00ff00ff) << 8) | ((objectId >> 8) & 0x00ff00ff);
	objectId = ((objectId & 0x0f0f0f0f) << 4) | ((objectId >> 4) & 0x0f0f0f0f);
	objectId = ((objectId & 0x33333333) << 2) | ((objectId >> 2) & 0x33333333);
	objectId = ((objectId & 0x55555555) << 1) | ((objectId >> 1) & 0x55555555);
	long id  = objectId;
	id <<= 2;
	id += instanceId;
	id <<= 14;
	id += resourceId;

	return id;
}

bool LwM2MClient::Notify(uint16_t objectId, uint16_t instanceId, uint16_t resourceId) {
	_i("Notify [%d] /%d/%d/%d", coap->getNextMessageId(), objectId, instanceId, resourceId);
	Observe *observe = (Observe *)observes->find(convertObserveId(objectId, instanceId, resourceId));
	if (!observe) {
		_i("No observed");
		return false;
	} else {
		_i("token: %lld, counter: %d, message id: %d", *((uint64_t *)observe->token), observe->counter, observe->messageId);
	}

	LwM2MInstance *instance = (LwM2MInstance *)instances->find(LwM2MInstance::getId(objectId, instanceId));
	if (!instance) {
		_i("No instance");
		return false;
	}

	size_t readLength = 0;
	if (instance->read(resourceId, readOperationBuffer, &readLength) != Coap::Code::Content) {
		_i("Read error %d", -readLength);
		return false;
	}

	Coap::Option options;
	options.format		   = 0x2d16;
	options.isObserve	   = true;
	options.observeCounter = observe->counter++;

	observe->messageId = coap->getNextMessageId();
	coap->Request(Coap::Type::NonConfirmable, Coap::Code::Content, observe->token)
	    ->Option(&options)
	    ->Payload(readOperationBuffer, readLength)
	    ->send(dtls);
	return true;
}

bool LwM2MClient::Send(const char *json) {
	Coap::Option options;
	options.format = 0x6e;  //2d17; // json (SenML JSON 0x6e)

	coap->Request(Coap::Type::NonConfirmable, Coap::Code::Content)
	    ->Option(&options)
	    ->Payload((const uint8_t *)json, strlen(json))
	    ->send(dtls);
	return true;
}

void LwM2MClient::CoapProcess(IConnection *connection, std::function<void()> timeoutCallback) {
	int result = coap->receive(connection);
	if (result == 0) {
		if (timeoutCallback) timeoutCallback();
		return;
	} else if (result < 0) {
		_i("Receive invalid packet");
		registered = false;
		return;
	}

	Coap::Type type = coap->getType();
	Coap::Code code = coap->getCode();
	Coap::Option option;

	size_t n;
	uint16_t objectId	= 0xffff;
	uint16_t instanceId = 0xffff;
	uint16_t resourceId = 0xffff;

	Observe *observe;

	_i("Receive coap packet, Type: %d, Code: %d", type, code);
	switch (type) {
		case Coap::Type::Acknowledgement:
			switch (code) {
				case Coap::Code::Created:
					// Registered
					coap->getOption(&option);

					strncpy(location, option.locations[1], 15);
					registered	  = true;
					updatedTimestamp = esp_timer_get_time();
					break;

				case Coap::Code::Changed:
					// Updated
					if (bootstraped) {
						_i("Updated");
						updatedTimestamp = esp_timer_get_time();
					} else {
						// bootstrap Request ACK
					}
					break;

				case Coap::Code::NotFound:
				case Coap::Code::BadRequest:
					_i("Bad request or Not Found");
					registered = false;
					vTaskDelay(120 * 1000 / portTICK_PERIOD_MS);
					break;

				case Coap::Code::Empty:
				case Coap::Code::Get:
				case Coap::Code::Post:
				case Coap::Code::Put:
				case Coap::Code::Delete:
				case Coap::Code::OK:
				case Coap::Code::Deleted:
				case Coap::Code::Valid:
				case Coap::Code::Content:
				case Coap::Code::Unauthorized:
				case Coap::Code::BadOption:
				case Coap::Code::Forbidden:
				case Coap::Code::MethodNotAllowed:
				case Coap::Code::NotAcceptable:
					break;
			}
			break;
		case Coap::Type::Confirmable:
			coap->getOption(&option);
			n = option.pathLen;
			if (n > 0) objectId = atoi(option.paths[0]);
			if (n > 1) instanceId = atoi(option.paths[1]);
			if (n > 2) resourceId = atoi(option.paths[2]);
			switch (code) {
				case Coap::Code::Get:
					if (option.isObserve) {
						observe		  = (Observe *)malloc(sizeof(Observe));
						observe->counter = 0;
						memcpy(observe->token, coap->getToken(), CoapPacket::TokenLength);

						// Published Observesに登録。古いデータが存在する場合削除。
						long id = convertObserveId(objectId, instanceId, resourceId);
						_i("Observe request [%ld] /%u/%u/%u, token: %llx MID: %u", id, objectId, instanceId, resourceId, *((uint64_t *)observe->token), observe->messageId);

						observe = (Observe *)observes->add(id, observe);
						if (observe) {
							_i("Delete old observe [%ld] token: %llx", id, *((uint64_t *)observe->token));
							free(observe);
						}
					}
					ReadResource(objectId, instanceId, resourceId);
					break;
				case Coap::Code::Post:
					if (bootstraped && n >= 3) {	// ExecuteはresourceIdまで特定が必要
						ExecuteResource(objectId, instanceId, resourceId);
					} else {
						BootstrapFinishRequest();
					}
					break;
				case Coap::Code::Put:
					WriteResource(objectId, instanceId, resourceId);
					break;
				case Coap::Code::Delete:
					DeleteRequest();
					break;
				case Coap::Code::Created:
				case Coap::Code::Changed:
				case Coap::Code::NotFound:
				case Coap::Code::BadRequest:
				case Coap::Code::Empty:
				case Coap::Code::OK:
				case Coap::Code::Deleted:
				case Coap::Code::Valid:
				case Coap::Code::Content:
				case Coap::Code::Unauthorized:
				case Coap::Code::BadOption:
				case Coap::Code::Forbidden:
				case Coap::Code::MethodNotAllowed:
				case Coap::Code::NotAcceptable:
					break;
			}
			break;
		case Coap::Type::NonConfirmable:
			break;
		case Coap::Type::Reset:
			uint16_t messageId = coap->getMessageId();

			void *removeObserve = observes->removeAt([&](void *data) {
				return ((Observe *)data)->messageId == messageId;
			});
			if (removeObserve) {
				_i("Observe unpublished %d", messageId);
				free(removeObserve);
			}

			break;
	}
}
