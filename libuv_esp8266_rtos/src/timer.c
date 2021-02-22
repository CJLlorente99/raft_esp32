#include "uv.h"


// Funcionamiento de los timers
// Repeat -> cada cuanto se debe repetir el timer (0 == one-shot)
// timeout -> tiempo para el proximo cb call

void run_timer(uv_handle_t* handle);

// virtual table for check handlers
static handle_vtbl_t timer_vtbl = {
    .run = run_timer
};

int
uv_timer_init (uv_loop_t* loop, uv_timer_t* handle){
    handle->self = malloc(sizeof(uv_handle_t));
    handle->self->loop = loop;
    handle->self->vtbl = &timer_vtbl;

    handle->timeout = 0;
    handle->repeat = 0;
    handle->timer_cb = NULL;
    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;

    uint64 clamped_timeout;

    // asi se trata en el libuv original (entiendo que es por si llegamos al limite de los 64 bits)
    clamped_timeout = loop->time + timeout;
    if(clamped_timeout < timeout){
        clamped_timeout = (uint64) -1;
    }

    handle->timer_cb = cb;
    handle->timeout = clamped_timeout;
    handle->repeat = repeat;

    insert_handle(loop, (uv_handle_t*)handle);

    // TODO
    // libuv original hace algo que no termino de entender bien (diria que es para ordenar los timers)

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;

    remove_handle(loop, (uv_handle_t*)handle);
    
    return 0;
}

int
uv_timer_again(uv_timer_t* handle){
    
    if(handle->repeat){
        uv_timer_stop(handle);
        uv_timer_start(handle, handle->timer_cb, handle->repeat, handle->repeat);
    }

    return 0;
}

void
run_timer(uv_handle_t* handle){
    uv_timer_t* timer = (uv_timer_t*) handle;
    loopFSM_t* loop = timer->self->loop->loopFSM->user_data;

    if(timer->timeout > loop->time)
        return;

    uv_timer_stop(timer);
    uv_timer_again(timer);
    timer->timer_cb(timer);
}