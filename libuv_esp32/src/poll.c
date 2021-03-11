#include "uv.h"

void run_poll(uv_handle_t* handle);

static handle_vtbl_t poll_vtbl = {
    .run = run_poll
};

int
uv_poll_init(uv_loop_t* loop, uv_poll_t* handle, int fd){
    handle->self->loop = loop;
    handle->self->vtbl = &poll_vtbl;

    return 0;
}

int
uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb){
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;
    int rv;
    
    handle->cb = cb;

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        ESP_LOGE("UV_POLL_START", "Error during uv_insert in uv_poll_start");
    }

    return 0;
}

int
uv_poll_stop(uv_poll_t* poll){
    loopFSM_t* loop = poll->self->loop->loopFSM->user_data;
    int rv;

    rv = uv_remove_handle(loop, (uv_handle_t*)poll);
    if(rv != 0){
        ESP_LOGE("UV_POLL_STOP", "Error during uv_remove in uv_poll_stop");
    }
    return 0;
}

void
run_poll(uv_handle_t* handle){

}