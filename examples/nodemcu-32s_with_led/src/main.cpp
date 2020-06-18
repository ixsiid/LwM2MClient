#include <driver/gpio.h>
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

	WiFi::Connect(SSID, PASSWORD);
	_i("IP: %s", inet_ntoa(*WiFi::getIp()));

	int led = 0;
	// LED点滅用GPIO初期化
	gpio_pad_select_gpio(BLINK_GPIO);
	gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)BLINK_GPIO, led);

	size_t a = esp_get_free_heap_size();

	int64_t time = esp_timer_get_time();
	_i("%llx", (uint64_t)time);

	LwM2MClient *lwm2m = LwM2MFactory("espidf2", 120)
						.Ip(WiFi::getIp())
						// Bootstrapする場合
						// .Bootstrap("bootstrap.soracom.io", 5683)

						// 払い出し済みのキーを使う場合
						.SetSecurityPram(DEVICE_KEY, (const uint8_t *)DEVICE_SECRET)

						// 続けて登録するインスタンスを追加する
						.AddInstance(new DeviceInstance(0))  // Object ID: 3
						.AddFixResource(2, "LwM2MClient Factory")
						.AddFixResource(10, a / 1024)
						.AddResource(4, [](Operations operation, TLVData *tlv) {
							_i("Execute parameter: %s", (const char *)tlv->row);
							esp_restart();
						})

						.AddInstance(new LightControlInstance(0))  // Object ID: 3311
						.AddResource(5852, [&](Operations operation, TLVData *tlv) {
							// On time
							if (operation == Read) {
								tlv->int32Value = (esp_timer_get_time() - time) / 1000000;
							} else if (operation == Write) {
								_i("%llx %llx", time, (uint64_t)tlv->int32Value);
								if (tlv->int32Value == 0) {
									time = esp_timer_get_time();
								}
							}
						})
						.AddResource(5850, [&](Operations operation, TLVData *tlv) {
							// On/Off
							if (operation == Read) {
								tlv->int8Value = led;
							} else if (operation == Write) {
								gpio_set_level((gpio_num_t)BLINK_GPIO, led = tlv->int8Value);
							}
						})

						.AddInstance(new LightControlInstance(5))  // Object ID: 3311
						.AddResource(5852, [&](Operations operation, TLVData *tlv) {
							// On time
							if (operation == Read) {
								tlv->int32Value = ((esp_timer_get_time() - time) / 1000000) * 3;
							} else if (operation == Write) {
								_i("%llx %llx", time, (uint64_t)tlv->int32Value);
								if (tlv->int32Value == 0) {
									time = esp_timer_get_time();
								}
							}
						})
						.AddResource(5850, [&](Operations operation, TLVData *tlv) {
							// On/Off
							if (operation == Read) {
								tlv->int8Value = led;
							} else if (operation == Write) {
								gpio_set_level((gpio_num_t)BLINK_GPIO, led = tlv->int8Value);
							}
						})

						.Regist(LWM2M_HOST, 5684);

	size_t b = esp_get_free_heap_size();

	_i("Heap %d -> %d (%d)", a, b, b - a);

	_i("LwM2M Client initialized: %p", lwm2m);
	xTaskCreatePinnedToCore([](void *lwm2m) {
		_i("Run LwM2M event loop on Core %d", xPortGetCoreID());
		while (true) ((LwM2MClient *)lwm2m)->CheckEvent();
	}, "EventLoop", 4096, lwm2m, 1, NULL, 0);

	// 周期Notifyループ
	xTaskCreatePinnedToCore([](void *lwm2m_p) {
		_i("Notify management on %d", xPortGetCoreID());
		LwM2MClient *lwm2m = (LwM2MClient *)lwm2m_p;

		uint32_t lastNotified = esp_timer_get_time();
		while (true) {
			// Core1はWatch Dog Timerを介入させるためにDelay挿入
			vTaskDelay(3000 / portTICK_PERIOD_MS);

			// 20秒以上経過していたらNotifyを送る
			uint32_t now = esp_timer_get_time();
			if (now - lastNotified > 20000000) {
				lastNotified = now;
				((LwM2MClient *)lwm2m)->Notify(3311, 0, 5852);
				((LwM2MClient *)lwm2m)->Notify(3311, 5, 5852);
			}
		}
	}, "Notify", 4096, lwm2m, 1, NULL, 1);
}
