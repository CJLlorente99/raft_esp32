#include "uv.h"

/* Run function, vtbl and uv_timer */
void
run_timer(uv_handle_t* handle){
    uv_timer_t* timer = (uv_timer_t*) handle;
    loopFSM_t* loop = timer->loop->loopFSM->user_data;
    int rv;

    if(timer->timeout > loop->time)
        return;

    rv = uv_timer_again(timer);
    if(rv != 0){
        ESP_LOGE("RUN_TIMER", "Error when calling uv_timer_again in run_timer");
    }
    timer->timer_cb(timer);
}

static handle_vtbl_t timer_vtbl = {
    .run = run_timer
};

int
uv_timer_init (uv_loop_t* loop, uv_timer_t* handle){

    handle->self.loop = loop;
    handle->self.type = UV_TIMER;
    handle->self.vtbl = &timer_vtbl;

    handle->loop = loop;
    handle->repeat = 0;
    handle->timeout = 0;
    handle->timer_cb = NULL;
    handle->type = UV_TIMER;

    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;
    int rv;
    uint32_t clamped_timeout;

    clamped_timeout = loop->time + (uint32_t)timeout;
    if(clamped_timeout < timeout){
        clamped_timeout = (uint32_t) -1;
    }

    handle->timer_cb = cb;
    handle->timeout = clamped_timeout;
    handle->repeat = (uint32_t)repeat;

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        ESP_LOGE("UV_TIMER_START", "Error when calling uv_insert_handle in uv_timer_start");
        return 1;
    }

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;
    int rv;

    rv = uv_remove_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        ESP_LOGE("UV_TIMER_STOP", "Error when calling uv_remove_handles in uv_timer_stop");
        return 1;
    }
    
    return 0;
}

int
uv_timer_again(uv_timer_t* handle){
    int rv;

    if(handle->repeat){
        rv = uv_timer_stop(handle);
        if(rv != 0){
            ESP_LOGE("UV_TIMER_AGAIN", "Error when calling uv_timer_stop in uv_timer_again");
            return 1;
        }

        rv = uv_timer_start(handle, handle->timer_cb, handle->repeat, handle->repeat);
        if(rv != 0){
            ESP_LOGE("UV_TIMER_AGAIN", "Error when calling uv_timer_start in uv_timer_again");
            return 1;
        }
    }

    return 0;
}