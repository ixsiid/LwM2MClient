#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>

#include <wifiManager.hpp>
#include "wifi_credential.h"
/*
#define SSID "your ssid"
#define PASSWORD "your password"
*/
#include "lwm2m_credential.h"
/*
#define DEVICE_KEY "your device key"

const char DEVICE_SECRET_KEY_BYTE_ARRAY[] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f}; // Your device secret
#define DEVICE_SECRET DEVICE_SECRET_KEY_BYTE_ARRAY

#define LWM2M_HOST "host ip address"
// DNS lookup機能はありません
*/

#include <lwm2mFactory.hpp>
#include <objects/3311_LightControl.hpp>
#include <objects/3_Device.hpp>


#define TAG "LwM2M Client"
#include "log.h"

using namespace LwM2M;
using namespace LwM2MObject;

extern "C" {
void app_main();
}

void app_main() {
	_i("Start LwM2MClient");

	WiFi::Connect(WIFI_SSID, WIFI_PASSWORD);
	_i("IP: %s", inet_ntoa(*WiFi::getIp()));

	int led = 0;
	// LED点滅用GPIO初期化
	gpio_pad_select_gpio(BLINK_GPIO);
	gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)BLINK_GPIO, led);

	int64_t time = esp_timer_get_time();

	LwM2MClient *lwm2m = LwM2MFactory("espidf2", 120)
						.Ip(WiFi::getIp())
						// Bootstrapする場合
						// .Bootstrap("bootstrap.soracom.io", 5683)

						// 払い出し済みのキーを使う場合
						.SetSecurityPram(SORACOM_DEVICE_KEY, (const uint8_t *)SORACOM_DEVICE_SECRET)

						// 続けて登録するインスタンスを追加する
						.AddInstance(new DeviceInstance(0)) // Object ID: 3
						.AddResource(2, [](Operations operation, TLVData *tlv, int length) {
							strcpy((char *)tlv->bytesValue, "123456789");
							tlv->dataLength = 9;
						})
						.AddResource(4, [](Operations operation, TLVData *tlv, int length) {
							_i("Execute parameter: %s", tlv->bytesValue);
							esp_restart();
						})

						.AddInstance(new LightControlInstance(0))  // Object ID: 3311
						.AddResource(5852, [&](Operations operation, TLVData *tlv, int length) {
							// On time
							if (operation == Read) {
								tlv->longValue	 = (esp_timer_get_time() - time) / 1000000;
								tlv->dataLength = 4;
							} else if (operation == Write) {
								if (tlv->byteValue == 0) {
									time = esp_timer_get_time();
								}
							}
						})
						.AddResource(5850, [&](Operations operation, TLVData *tlv, int length) {
							// On/Off
							if (operation == Read) {
								tlv->byteValue	 = led;
								tlv->dataLength = 1;
							} else if (operation == Write) {
								led = tlv->byteValue;
								gpio_set_level((gpio_num_t)BLINK_GPIO, led);
							}
						})

						.Regist(LWM2M_HOST, 5684);

	_i("LwM2M Client initialized: %p", lwm2m);
	xTaskCreatePinnedToCore([](void *lwm2m) {
		_i("Run LwM2M event loop on Core %d", xPortGetCoreID());
		while (true) ((LwM2MClient *)lwm2m)->CheckEvent();
	}, "EventLoop", 4096, lwm2m, 1, NULL, 0);

	// 周期Notifyループ
	xTaskCreatePinnedToCore([](void *lwm2m) {
		_i("Notify management on %d", xPortGetCoreID());
		uint32_t lastNotified = esp_timer_get_time();
		while (true) {
			// Core1はWatch Dog Timerを介入させるためにDelay挿入
			vTaskDelay(3000 / portTICK_PERIOD_MS);

			// 20秒以上経過していたらNotifyを送る
			uint32_t now = esp_timer_get_time();
			if (now - lastNotified > 20000000) {
				lastNotified = now;
				((LwM2MClient *)lwm2m)->Notify(3311, 0, 5852);
			}
		}
	}, "Notify", 4096, lwm2m, 1, NULL, 1);
}
