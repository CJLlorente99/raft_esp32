/* This file should contain some code that verifies libuv implementation correctness
To be verified
    signals -> some buttons and led (when one is pressed all leds on, when the other is pressed all leds off)

*/

#include "uv.h"
#include "init.h"

// SIGNAL TEST

#define INPUT_TEST_PORT_OFF 22
#define INPUT_TEST_PORT_ON 19
#define LED_TEST_PORT 14
// #define LED_DEBUG_PORT 5

void
test_callback_on (uv_signal_t* handle, int signum){
    gpio_set_level(LED_TEST_PORT,1);
}

void
test_callback_off (uv_signal_t* handle, int signum){
    gpio_set_level(LED_TEST_PORT,0);
}

void
main_signal(void* ignore){
    // Configure GPIO
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = 1ULL<<LED_TEST_PORT;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL<<INPUT_TEST_PORT_OFF)|(1ULL<<INPUT_TEST_PORT_ON));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_signal");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_signal");

    // Init signal handle
    uv_signal_t* signal_handle_on = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_handle_off = malloc(sizeof(uv_signal_t));

    rv = uv_signal_init(loop, signal_handle_on);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la primera inicializacion en main_signal");
    }

    ESP_LOGI("SIGNAL_INIT", "Primer signal inicializado en main_signal");

    // Aqui hay un error
    rv = uv_signal_start(signal_handle_on, test_callback_on, INPUT_TEST_PORT_ON);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante primer uv_signal_start en main_signal");
    }

    ESP_LOGI("SIGNAL_START", "Primer uv_signal_start en main_signal");

    rv = uv_signal_init(loop, signal_handle_off);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la segunda inicializacion en main_signal");
    }

    ESP_LOGI("SIGNAL_INIT", "Segundo signal inicializado en main_signal");

    rv = uv_signal_start(signal_handle_off, test_callback_off, INPUT_TEST_PORT_OFF);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante segundo uv_signal_start en main_signal");
    }

    ESP_LOGI("SIGNAL_START", "Segundo uv_signal_start en main_signal");

    rv = uv_run(loop);
    if(rv != 0){
        ESP_LOGE("UV_RUN","Error durante uv_run");
    }

    vTaskDelete(NULL);
}

// CHECK TEST

#define LED_CHECK_TEST_PORT 14
bool led_state = 0;

// #define LED_DEBUG_PORT 5

void
check_callback (uv_check_t* handle){
    gpio_set_level(LED_CHECK_TEST_PORT,(int)led_state);
    led_state = !led_state;
}


void
main_check(void* ignore){
    // Configure GPIO
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = 1ULL<<LED_CHECK_TEST_PORT;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_signal");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_signal");

    // Init signal handle
    uv_check_t* check_handle = malloc(sizeof(uv_check_t));

    rv = uv_check_init(loop, check_handle);
    if(rv != 0){
        ESP_LOGE("CHECK_INIT","Error durante la primera inicializacion en main_check");
    }

    ESP_LOGI("CHECK_INIT", "Primer signal inicializado en main_check");

    // Aqui hay un error
    rv = uv_check_start(check_handle, check_callback);
    if(rv != 0){
        ESP_LOGE("CHECK_START","Error durante primer uv_check_start en main_check");
    }

    ESP_LOGI("CHECK_START", "Primer uv_check_start en main_check");

    rv = uv_run(loop);
    if(rv != 0){
        ESP_LOGE("UV_RUN","Error durante uv_run");
    }

    vTaskDelete(NULL);
}

// TIMER TEST

// #define ONE_SHOT_CLOCK_TRIGGER_PORT 15
// #define LED_ONE_SHOT_PORT 17
// #define LED_TWO_SEC_PORT 18
#define LED_HALF_SEC_PORT 2

void
half_sec_callback_on (uv_timer_t* handle){
    gpio_set_level(LED_HALF_SEC_PORT,1);
    ESP_LOGI("LED", "LED ENCENDIDA");
}

void
half_sec_callback_off (uv_timer_t* handle){
    gpio_set_level(LED_HALF_SEC_PORT,0);
    ESP_LOGI("LED", "LED APAGADA");
}

void
main_timer(void* ignore){ 

    // Configure GPIO
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<LED_HALF_SEC_PORT);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_signal");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_signal");

    // Init timers
    uv_timer_t* timer_half_sec_on = malloc(sizeof(uv_timer_t));
    uv_timer_t* timer_half_sec_off = malloc(sizeof(uv_timer_t));

    rv = uv_timer_init(loop, timer_half_sec_on);
    if(rv != 0){
        ESP_LOGE("TIMER_INIT", "Error in first uv_timer_init in main_timer");
    }

    ESP_LOGI("TIMER_INIT", "First uv_timer_init success");

    rv = uv_timer_init(loop, timer_half_sec_off);
    if(rv != 0){
        ESP_LOGE("TIMER_INIT", "Error in second uv_timer_init in main_timer");
    }

    ESP_LOGI("TIMER_INIT", "Second uv_timer_init success");

    rv = uv_timer_start(timer_half_sec_on, half_sec_callback_on, 0, 2000);
    if(rv != 0){
        ESP_LOGE("TIMER_START", "Error in first uv_timer_start in main_timer");
    }

    ESP_LOGI("TIMER_START", "First uv_timer_start success");

    rv = uv_timer_start(timer_half_sec_off, half_sec_callback_off, 1000, 2000);
    if(rv != 0){
        ESP_LOGE("TIMER_START", "Error in second uv_timer_start in main_timer");
    }

    ESP_LOGI("TIMER_START", "Second uv_timer_start success");

    rv = uv_run(loop);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_timer");
    }

    vTaskDelete(NULL);
}

// TCP test server

#define LOCALIP "192.168.0.100"
#define CLIENTIP "192.168.0.101"

uv_tcp_t* tcp_server;

void
connection_cb(uv_stream_t* server, int status){
    int rv;
    ESP_LOGI("CONNECTION_CB", "Connection callback has been called with status = %d", status);
    
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(CLIENTIP);
    client_addr.sin_port = htons(5000);

    uv_stream_t* client = malloc(sizeof(uv_stream_t));
    memset(client, 0, sizeof(uv_stream_t));
    client->src_sockaddr = (struct sockaddr*)&client_addr;
    rv = uv_accept(server, client);
    if(rv != 0){
        ESP_LOGE("CONNECTION_CB", "Error while trying to accept");
    }
}

void
write_cb(uv_write_t* req, int status){
    ESP_LOGI("WRITE_CB", "Write callback has been called with status = %d", status);
}

void
tcp_timer_write_cb(uv_timer_t* handle){
    int rv;
    uv_write_t* req = malloc(sizeof(uv_write));
    uv_buf_t buf;
    buf.base = "Saludos";
    buf.len = sizeof(buf.base);

    rv = uv_write(req, (uv_stream_t*)tcp_server, &buf, 1, write_cb);
    if(rv != 0){
        ESP_LOGE("TCP_TIMER_CB","Error while trying to write");
    }
}

void
main_tcp_server(void* ignore){ 

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_signal");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_signal");

    // Init timers
    tcp_server = malloc(sizeof(uv_tcp_t));

    rv = uv_tcp_init(loop, tcp_server);
    if(rv != 0){
        ESP_LOGE("TCP_INIT", "Error in first uv_tcp_init in main_tcp_server");
    }

    ESP_LOGI("TCP_INIT", "First uv_tcp_init success");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = inet_addr(LOCALIP);

    rv = uv_tcp_bind(tcp_server, (struct sockaddr*)&addr, 0);
    if(rv != 0){
        ESP_LOGE("TCP_BIND", "Error in uv_tcp_bind in main_tcp_server");
    }

    ESP_LOGI("TCP_BIND", "uv_tcp_bind success");

    rv = uv_listen((uv_stream_t*)tcp_server, 1, connection_cb);
    if(rv != 0){
        ESP_LOGE("UV_LISTEN", "Error in uv_listen in main_tcp_server");
    }

    ESP_LOGI("UV_LISTEN", "uv_listen success");

    uv_timer_t* timer = malloc(sizeof(uv_timer_t));
    
    rv = uv_timer_init(loop, timer);
    if(rv != 0){
        ESP_LOGE("TIMER_INIT", "Error uv_timer_init in main_tcp_server");
    }

    ESP_LOGI("TIMER_INIT", "Second uv_timer_init success");

    rv = uv_timer_start(timer, tcp_timer_write_cb, 0, 2000);
    if(rv != 0){
        ESP_LOGE("TIMER_START", "Error in first uv_timer_start in main_timer");
    }

    rv = uv_run(loop);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_timer");
    }

    vTaskDelete(NULL);
}

// TCP test client

uv_tcp_t* tcp_client;

void
alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    buf->len = suggested_size;
    // malloc buf->base
}

void
read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
    if(nread < 0)
        return;

    if(nread == buf->len){ // stop reading, everything expected has been read
        ESP_LOGI("READ_CB", "Everything received = %s", buf->base);
        uv_read_stop(stream);
        return;
    } else if(nread < buf->len){ // more info is going to be read, well be called afterwards
        return;
    }
}

void
connect_cb(uv_connect_t* req, int status){
    ESP_LOGI("CONNECT_CB", "Entered connect_cb with status = %d", status);
}

void
tcp_timer_read_cb(uv_timer_t* handle){
    int rv;

    rv = uv_read_start((uv_stream_t*)tcp_client, alloc_cb, read_cb);
    if(rv != 0){
        ESP_LOGE("TCP_TIMER_READ_CB","Error while trying to read_start");
    }
}

void
main_tcp_client(void* ignore){ 

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_tcp_client");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_tcp_client");

    // Init timers
    tcp_client = malloc(sizeof(uv_tcp_t));

    rv = uv_tcp_init(loop, tcp_client);
    if(rv != 0){
        ESP_LOGE("TCP_INIT", "Error in first uv_tcp_init in main_tcp_client");
    }

    ESP_LOGI("TCP_INIT", "First uv_tcp_init success");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = inet_addr(CLIENTIP);

    rv = uv_tcp_bind(tcp_client, (struct sockaddr*)&addr, 0);
    if(rv != 0){
        ESP_LOGE("TCP_BIND", "Error in uv_tcp_bind in main_tcp_client");
    }

    ESP_LOGI("TCP_BIND", "uv_tcp_bind success");

    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = inet_addr(LOCALIP);

    rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
    if(rv != 0){
        ESP_LOGE("TCP_CONNECT", "Error in uv_tcp_connect in main_tcp_client");
    }

    ESP_LOGI("TCP_CONNECT", "uv_tcp_connect success");

    uv_timer_t* timer = malloc(sizeof(uv_timer_t));
    
    rv = uv_timer_init(loop, timer);
    if(rv != 0){
        ESP_LOGE("TIMER_INIT", "Error uv_timer_init in main_tcp_server");
    }

    ESP_LOGI("TIMER_INIT", "Second uv_timer_init success");

    rv = uv_timer_start(timer, tcp_timer_read_cb, 0, 2000);
    if(rv != 0){
        ESP_LOGE("TIMER_START", "Error in first uv_timer_start in main_timer");
    }

    rv = uv_run(loop);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_timer");
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI("APP_MAIN", "Beggining app_main");
    vTaskDelay(5000/portTICK_RATE_MS);

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // wifi_init();
    // is it necessary to change to static IP from dhcp?
    // either use lwip to disable dhcp and set static ip
    // or esp_netif to do the same

    xTaskCreate(main_signal, "startup", 4096, NULL, 5, NULL);
    // xTaskCreate(main_check, "startup", 4096, NULL, 5, NULL);
    // xTaskCreate(main_timer, "startup", 4096, NULL, 5, NULL);
    // xTaskCreate(main_tcp_server, "startup", 4096, NULL, 5, NULL);
    // xTaskCreate(main_tcp_client, "startup", 4096, NULL, 5, NULL);
}