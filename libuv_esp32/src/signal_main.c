#include "uv.h"

#define INPUT_TEST_PORT_OFF 22
#define INPUT_TEST_PORT_ON 19

/* Callbacks linked to each signal */
void
test_callback_on (uv_signal_t* handle, int signum){
    ESP_LOGI("test_callback_on","");
}

void
test_callback_off (uv_signal_t* handle, int signum){
    ESP_LOGI("test_callback_off","");
}

/* main */
void
main_signal(void* ignore){
    /* Configure GPIO */
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL<<INPUT_TEST_PORT_OFF)|(1ULL<<INPUT_TEST_PORT_ON));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    /* Init loop */
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("main_signal","uv_loop_init");
    }

    /* Init signal handles */
    uv_signal_t* signal_handle_on = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_handle_off = malloc(sizeof(uv_signal_t));

    rv = uv_signal_init(loop, signal_handle_on);
    if(rv != 0){
        ESP_LOGE("main_signal","uv_signal_init: signal_handle_on");
    }

    rv = uv_signal_init(loop, signal_handle_off);
    if(rv != 0){
        ESP_LOGE("main_signal","uv_signal_init: signal_handle_off");
    }    
    
    rv = uv_signal_start(signal_handle_on, test_callback_on, INPUT_TEST_PORT_ON);
    if(rv != 0){
        ESP_LOGE("main_signal","uv_signal_start: signal_handle_on");
    }

    rv = uv_signal_start(signal_handle_off, test_callback_off, INPUT_TEST_PORT_OFF);
    if(rv != 0){
        ESP_LOGE("main_signal","uv_signal_start: signal_handle_on");
    }

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("main_signal","Error durante uv_run");
    }

    vTaskDelete(NULL);
}