#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>

#include <wifiManager.hpp>

#include "wifi_credential.h"
#include "lwm2m_credential.h"

#include <lwm2mFactory.hpp>
#include <objects/3_Device.hpp>
#include <objects/3322_Load.hpp>

#define TAG "LwM2M Client"
#include "log.h"

using namespace LwM2M;
using namespace LwM2MObject;

extern "C" {
void app_main();
}


static const size_t scaleCount = 2;

static double values[scaleCount];
static double sum;

void app_main() {
	_i("Start LwM2MClient");

	WiFi::Connect(SSID, PASSWORD);
	_i("IP: %s", inet_ntoa(*WiFi::getIp()));

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
							_i("Execute parameter: %s", (const char *)tlv->raw);
							esp_restart();
						})

						.AddInstance(new LoadInstance(3))
						.AddResource(5700, [](Operations operation, TLVData *tlv) {
							if (operation == Read) {
								tlv->floatValue = sum;
							}
						})
						.AddInstance(new LoadInstance(1))
						.AddResource(5700, [&](Operations operation, TLVData *tlv) {
							if (operation == Read) {
								tlv->floatValue = values[0];
							}
						})

						.AddInstance(new LoadInstance(2))
						.AddResource(5700, [&](Operations operation, TLVData *tlv) {
							if (operation == Read) {
								tlv->floatValue = values[1];
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

	/*
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
	*/
}
