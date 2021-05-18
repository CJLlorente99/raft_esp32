#include "uv.h"

/* Run function, vtbl and uv_timer */
void
run_timer(uv_handle_t* handle){
    // ESP_LOGI("run_timer","entering");
    uv_timer_t* timer = (uv_timer_t*) handle;
    loopFSM_t* loop = timer->loop->loop;
    int rv;

    if(timer->timeout > (uint64_t)loop->time)
        return;

    handle->data = timer->data;
    timer->timer_cb(timer);

    rv = uv_timer_again(timer);
    if(rv != 0){
        ESP_LOGE("RUN_TIMER", "Error when calling uv_timer_again in run_timer");
    }

    // ESP_LOGI("run_timer","exiting");
}

static handle_vtbl_t timer_vtbl = {
    .run = run_timer
};

int
uv_timer_init (uv_loop_t* loop, uv_timer_t* handle){

    handle->self.active = 0;
    handle->self.loop = loop;
    handle->self.type = UV_TIMER;
    handle->self.vtbl = &timer_vtbl;
    handle->self.remove = 0;

    handle->data = NULL;
    handle->loop = loop;
    handle->repeat = 0;
    handle->timeout = 0;
    handle->timer_cb = NULL;
    handle->type = UV_TIMER;

    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->loop->loop;
    int rv;
    uint64_t clamped_timeout;

    clamped_timeout = (uint64_t)loop->time + timeout;
    if(clamped_timeout < timeout){
        clamped_timeout = (uint64_t) -1;
    }

    handle->self.active = 1;
    handle->timer_cb = cb;
    handle->timeout = clamped_timeout;
    handle->repeat = repeat;

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        ESP_LOGE("UV_TIMER_START", "Error when calling uv_insert_handle in uv_timer_start");
        return 1;
    }

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){

    handle->self.active = 0;
    
    return 0;
}

int
uv_timer_again(uv_timer_t* handle){
    
    if(handle->repeat){
        uint64_t clamped_timeout = (uint64_t)handle->loop->loop->time + handle->repeat;
        if(clamped_timeout < handle->timeout){
            clamped_timeout = (uint64_t) -1;
        }

        handle->timeout = clamped_timeout;
    }
    return 0;
}