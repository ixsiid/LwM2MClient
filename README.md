# LwM2MClient

LwM2M Client for ESP-IDF

Based on easi (https://github.com/1stship/easi)

上記のリポジトリをC++に書き直しました。
また対応していなかったObserveにも対応しています。

```cpp:main.cpp

void app_main() {
	LwM2MClient *lwm2m = LwM2MFactory("espidf2", 120)
						.Ip(WiFi::getIp())
						.SetSecurityPram(SORACOM_DEVICE_KEY, (const uint8_t *)SORACOM_DEVICE_SECRET)
                        
						.AddInstance(new DeviceInstance(0)) // Object ID: 3
						.AddResource(2, [](Operations operation, TLVData *tlv, int length) {
							strcpy((char *)tlv->bytesValue, "123456789");
							tlv->dataLength = 9;
						})
						.AddResource(4, [](Operations operation, TLVData *tlv, int length) {
							_i("Execute parameter: %s", tlv->bytesValue);
							esp_restart();
						})

						.Regist(LWM2M_HOST, 5684);

	xTaskCreatePinnedToCore([](void *lwm2m) {
		while (true) ((LwM2MClient *)lwm2m)->CheckEvent();
	}, "EventLoop", 4096, lwm2m, 1, NULL, 0);
}
```
