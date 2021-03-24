#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

#define ESP_WIFI_SSID   "IRECHAR"
#define ESP_WIFI_PASS   "24051801"
#define GATEWAY_ADDR    "192.168.0.103"
#define NETMASK         "255.255.255.0"
#define ESP_MAXIMUM_RETRY   5

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT   BIT1

void wifi_init(char* ip_addr);