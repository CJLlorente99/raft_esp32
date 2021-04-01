#include "uv.h"

void run_poll(uv_handle_t* handle);

static handle_vtbl_t poll_vtbl = {
    .run = run_poll
};

int
uv_poll_init(uv_loop_t* loop, uv_poll_t* handle, int fd){
    handle->loop = loop;
    handle->self.vtbl = &poll_vtbl;
    handle->events = 0;
    handle->cb = NULL;
    handle->fd = fd;
    handle->type = UV_POLL;

    FD_ZERO(&handle->readset);
    FD_ZERO(&handle->writeset);

    FD_SET(fd, &handle->readset);
    FD_SET(fd, &handle->writeset);

    return 0;
}

int
uv_poll_start(uv_poll_t* handle, int events, uv_poll_cb cb){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;
    int rv;
    
    handle->cb = cb;
    handle->events = events;

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        ESP_LOGE("UV_POLL_START", "Error during uv_insert in uv_poll_start");
    }

    return 0;
}

int
uv_poll_stop(uv_poll_t* poll){
    loopFSM_t* loop = poll->loop->loopFSM->user_data;
    int rv;

    rv = uv_remove_handle(loop, (uv_handle_t*)poll);
    if(rv != 0){
        ESP_LOGE("UV_POLL_STOP", "Error during uv_remove in uv_poll_stop");
    }
    return 0;
}

void
run_poll(uv_handle_t* handle){
    uv_poll_t* poll_handle = (uv_poll_t*) handle;
    int status = 0;

    if(poll_handle->events == UV_READABLE){
        if(select(poll_handle->fd, &poll_handle->readset, NULL, NULL, 0))
            poll_handle->cb(poll_handle, status, poll_handle->events);
    }
}