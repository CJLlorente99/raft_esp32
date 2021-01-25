#include "uv.h"

int
uv_timer_init (uv_loop_t* loop_s, uv_timer_t* handle){
    handle->loop = loop_s;
    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;

    /*
    if(uv__is_closing(handle)) || cb == NULL)
        return UV_EINVAL;

    // Check if same handle reference exists. If it does, stop the reference to reconfigure it (?)
    if(uv__is_active(handle))
        uv_timer_stop(handle);

    */
    handle->timer_cb = cb;
    handle->timeout = timeout;
    handle->repeat = repeat;

    // If we have achieved to get here, create new handle and add it to signal_handlers 
    uv_timer_t** handlers = loop->active_timer_handlers;
    int i = loop->n_active_timer_handlers; // array index

    if(loop->n_active_timer_handlers == 0){
    *handlers = malloc(sizeof(uv_timer_t));
    memcpy((uv_timer_t*)handlers[0], handle, sizeof(uv_timer_t));
    } else {
    *handlers = realloc(*handlers, sizeof(uv_timer_t));
    memcpy((uv_timer_t*)handlers[i], handle, sizeof(uv_timer_t));
    }

    loop->n_active_timer_handlers++;

    // Activate timer

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    /*
    if(!uv__is_active(handle))
        return 0;
    */

    loopFSM_t* loop = handle->loop->loopFSM->user_data;

    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_timer_handlers - 1;
    uv_timer_t** new_handlers = malloc(sizeof(uv_timer_t[new_n_active_handlers]));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_timer_handlers; i++){
    if(loop->active_timer_handlers[i] != handle){
        memcpy((uv_timer_t*)new_handlers[j], loop->active_timer_handlers[i], sizeof(uv_timer_t));
        j++;
    }
    }

    // Exchange in loop structure
    loop->active_timer_handlers = new_handlers;

    // Delete timer (xTimerDelete())
}