#include "uv.h"

/* Create callbacks */
void
two_sec_callback (uv_timer_t* handle){
    ESP_LOGI("two_sec_callback", "");
}

void
half_sec_callback (uv_timer_t* handle){
    ESP_LOGI("half_sec_callback", "");
}

void
delayed_two_sec_callback (uv_timer_t* handle){
    ESP_LOGI("delayed_two_sec_callback", "");
}

void
end_callback (uv_timer_t* handle){
    ESP_LOGI("end_callback", "");

    uv_timer_t** active_timers = handle->data;

    for(int i = 0; i < 3; i++){
        uv_timer_stop(active_timers[i]);
        uv_close((uv_handle_t*)active_timers[i], NULL);
    }

    uv_timer_stop(handle);
    uv_close((uv_handle_t*)handle, NULL);

    ESP_LOGI("end_callback", "everything closed");
}

/* main */
void
main_timer(void* ignore){ 

    /* Init loop */
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("main_timer","Error durante la inicializacion en main_signal");
    }

    /* Init timers */
    uv_timer_t* timer_half_sec = malloc(sizeof(uv_timer_t));
    uv_timer_t* timer_two_sec = malloc(sizeof(uv_timer_t));
    uv_timer_t* timer_two_sec_delayed = malloc(sizeof(uv_timer_t));
    uv_timer_t* timer_end = malloc(sizeof(uv_timer_t));

    rv = uv_timer_init(loop, timer_half_sec);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_init: timer_half_sec");
    }

    rv = uv_timer_init(loop, timer_two_sec);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_init:  timer_two_sec");
    }

    rv = uv_timer_init(loop, timer_two_sec_delayed);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_init: timer_two_sec_delayed");
    }

    rv = uv_timer_init(loop, timer_end);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_init: timer_end");
    }

    rv = uv_timer_start(timer_half_sec, half_sec_callback, 0, 500);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_start: timer_half_sec");
    }

    rv = uv_timer_start(timer_two_sec, two_sec_callback, 0, 2000);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_start: timer_two_sec");
    }

    rv = uv_timer_start(timer_two_sec_delayed, delayed_two_sec_callback, 1000, 2000);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_start: timer_two_sec_delayed");
    }

    rv = uv_timer_start(timer_end, end_callback, 10000, 0);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_timer_start: timer_end");
    }

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("main_timer", "uv_run");
    }

    vTaskDelete(NULL);
}