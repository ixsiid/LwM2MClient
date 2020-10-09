#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>

#include <esp_sleep.h>

#include <Button.hpp>
#include <ili9341.hpp>
#include <lwm2mFactory.hpp>
#include <objects/3311_LightControl.hpp>
#include <objects/3322_Load.hpp>
#include <objects/3_Device.hpp>
#include <wifiManager.hpp>

#include "lwm2m_credential.hpp"
#include "wifi_credential.hpp"

#include "soracomDevice.hpp"

#define TAG "LwM2M Client"
#include "log.h"

using namespace LwM2M;
using namespace LwM2MObject;

using namespace LCD;

extern "C" {
void app_main();
}

static const size_t scaleCount = 2;
static float values[scaleCount];

static DeviceInstance *device;

void app_main() {
	_i("Start LwM2MClient");

	size_t a = esp_get_free_heap_size();

	ILI9341 *lcd = new ILI9341();

	size_t _lcd_a = esp_get_free_heap_size();
	_i("heap: %d -> %d (%d)", a, _lcd_a, _lcd_a - a);

	WiFi::Connect(SSID, PASSWORD);
	_i("IP: %s", inet_ntoa(*WiFi::getIp()));

	size_t _wifi_a = esp_get_free_heap_size();

	SoracomDevice *soracom = new SoracomDevice("jp.inventory.soracom.io", DEVICE_KEY, DEVICE_SECRET);
	_i("soracom host: %s", soracom->getHostStr());

	int64_t time = esp_timer_get_time();
	_i("%llx", (uint64_t)time);

	LwM2MClient *lwm2m = LwM2MFactory("espidf2", 120)
						.Ip(WiFi::getIp())
						// Bootstrapする場合
						// .Bootstrap("bootstrap.soracom.io", 5683)

						// 払い出し済みのキーを使う場合
						.SetSecurityPram(soracom->getDeviceKey(), soracom->getDeviceSecret())

						// 続けて登録するインスタンスを追加する
						.AddInstance(device = new DeviceInstance())	// Object ID: 3, Instance ID: 0
						.AddResource(0, (void *)"Kyocera Corp")
						.AddResource(1, (void *)"LwM2M client test")
						.AddResource(2, (void *)"1235.4312U-41TV")
						.AddResource(4, [](Operations operation, TLVData *tlv) {
							_i("Execute parameter: %s", (const char *)tlv->raw);
							esp_restart();
							return true;
						})
						.AddResource(10, [=](Operations operation, TLVData *tlv) {
							tlv->int32Value = esp_get_free_heap_size() / 1024;
							return true;
						})

						.AddInstance(new LoadInstance(1))
						.AddResource(5700, [](Operations operation, TLVData *tlv) {
							tlv->floatValue = values[0];
							return true;
						})
						.AddResource(5701, (void *)"12345678")
						.AddResource(5750, (void *)"Load Cell 1")

						.AddInstance(new LoadInstance(2))
						.AddResource(5700, [](Operations operation, TLVData *tlv) {
							tlv->floatValue = values[1];
							return true;
						})
						.AddResource(5701, (void *)"87654321")
						.AddResource(5750, (void *)"Load Cell 2")

						.AddInstance(new LoadInstance(3))
						.AddResource(5700, [](Operations operation, TLVData *tlv) {
							tlv->floatValue = values[0] + values[1];
							return true;
						})
						.AddResource(5701, (void *)"1234")
						.AddResource(5750, (void *)"Load Cell Sum")

						.AddInstance(new LightControlInstance(0))
						.AddResource(5852, [&](Operations operation, TLVData *tlv) {
							if (operation == Operations::Read) {
								tlv->int32Value = (esp_timer_get_time() - time) / 1000000;
								return true;
							} else if (operation == Operations::Write) {
								if (tlv->int32Value == 0) time = esp_timer_get_time();
								return true;
							}
							return false;
						})
						.AddResource(5701, (void *)"lux")

						.Regist(soracom->getHostStr(), 5684);

	device->addError(Error::LowBattery);
	device->addError(Error::GPSFailure);
	device->addError(Error::OutOfMemory);

	values[0] = values[1] = 0;
	time				  = esp_timer_get_time();

	size_t _lwm2m_a = esp_get_free_heap_size();

	_i("Heap %d -> %d (%d)", _wifi_a, _lwm2m_a, _lwm2m_a - _wifi_a);

	_i("LwM2M Client initialized: %p", lwm2m);
	xTaskCreatePinnedToCore([](void *lwm2m) {
		_i("Run LwM2M event loop on Core %d", xPortGetCoreID());
		while (true) ((LwM2MClient *)lwm2m)->CheckEvent();
	}, "EventLoop", 8192, lwm2m, 1, NULL, 0);

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
				lwm2m->Notify(3311, 0, 5852);

				lwm2m->Notify(3322, 1, 5700);
				lwm2m->Notify(3322, 2, 5700);
				lwm2m->Notify(3322, 3, 5700);

				values[0] += 0.24351f;
				values[1] += 0.5312f;
			}
		}
	}, "Notify", 4096, lwm2m, 1, NULL, 1);

	xTaskCreatePinnedToCore([](void *lcd_p) {
		ILI9341 *lcd = (ILI9341 *)lcd_p;
		char buf[64];

		while (true) {
			vTaskDelay(300 / portTICK_PERIOD_MS);

			lcd->clear(BLACK);
			sprintf(buf, "%lf", values[0]);
			lcd->drawString(2, 10, WHITE, buf);
			sprintf(buf, "%lf", values[1]);
			lcd->drawString(2, 30, WHITE, buf);
			sprintf(buf, "%lf", values[0] + values[1]);
			lcd->drawString(2, 60, WHITE, buf);
			lcd->update();
		}
	}, "LCDUpdate", 4096, lcd, 1, NULL, 1);
}
