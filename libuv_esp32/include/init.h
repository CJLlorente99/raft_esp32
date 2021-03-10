#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

#define ESP_WIFI_SSID   "ssid"
#define ESP_WIFI_PASS   "password"
#define ESP_MAXIMUM_RETRY   5

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT   BIT1

void wifi_init();