#include "uv.h"

// Funcionamiento de los timers
// Repeat -> cada cuanto se debe repetir el timer (0 == one-shot)
// timeout -> tiempo para el proximo cb call

int
uv_timer_init (uv_loop_t* loop, uv_timer_t* handle){
    handle->loop = loop;
    handle->self->handle_timer = handle;
    handle->self->type = TIMER;
    handle->timeout = 0;
    handle->repeat = 0;
    handle->timeout = 0;
    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;

    uint64 clamped_timeout;

    // asi se trata en el libuv original (entiendo que es por si llegamos al limite de los 64 bits)
    clamped_timeout = loop->time + timeout;
    if(clamped_timeout < timeout){
        clamped_timeout = (uint64) -1;
    }

    handle->timer_cb = cb;
    handle->timeout = clamped_timeout;
    handle->repeat = repeat;

    // Add to loop active handlers
    uv_handle_t** handlers = loop->active_handlers;
    int i = loop->n_active_handlers; // array index

    if(loop->n_active_handlers == 0){
        *handlers = malloc(sizeof(uv_signal_t));
        memcpy((uv_handle_t*)handlers[0], handle->self, sizeof(uv_handle_t));
    } else {
        *handlers = realloc(*handlers, sizeof(uv_handle_t[i]));
        memcpy((uv_handle_t*)handlers[i], handle->self, sizeof(uv_handle_t));
    }

    loop->n_active_handlers++;

    // TODO
    // libuv original hace algo que no termino de entender bien (diria que es para ordenar los timers)

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;

    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_handlers--;
    uv_handle_t** new_handlers = malloc(sizeof(uv_handle_t[new_n_active_handlers]));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->type == TIMER){
            if(loop->active_handlers[i]->handle_timer != handle){
                memcpy((uv_handle_t*)new_handlers[j++], loop->active_handlers[i], sizeof(uv_handle_t));
            }
        }
    }

    // Exchange in loop structure
    loop->active_handlers = new_handlers;
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