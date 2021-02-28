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

    handle->self.loop = loop;
    handle->self.vtbl = &timer_vtbl;

    handle->timeout = 0;
    handle->repeat = 0;
    handle->timer_cb = NULL;
    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->self.loop->loopFSM->user_data;

    int rv;
    uint32_t clamped_timeout;

    // asi se trata en el libuv original (entiendo que es por si llegamos al limite de los 64 bits)
    clamped_timeout = loop->time + (uint32_t)timeout;
    if(clamped_timeout < timeout){
        clamped_timeout = (uint32_t) -1;
    }

    handle->timer_cb = cb;
    handle->timeout = clamped_timeout;
    handle->repeat = (uint32_t)repeat;

    printf("Timeout is %d, repeat is %d\n", (uint32_t)handle->timeout, handle->repeat);

    rv = insert_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        printf("Error durante insert handle en timer start");
        return 1;
    }

    // TODO
    // libuv original hace algo que no termino de entender bien (diria que es para ordenar los timers)

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    loopFSM_t* loop = handle->self.loop->loopFSM->user_data;
    int rv;

    rv = remove_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        printf("Error en timer stop por remove handle\n");
        return 1;
    }
    
    return 0;
}

int
uv_timer_again(uv_timer_t* handle){
    int rv;

    printf("Volviendo a crear nuevo timer\n");

    if(handle->repeat){
        rv = uv_timer_stop(handle);
        if(rv != 0){
            printf("Error en timer stop durante timer again\n");
            return 1;
        }
        printf("Timer stop durante timer again\n");

        rv = uv_timer_start(handle, handle->timer_cb, handle->repeat, handle->repeat);
        if(rv != 0){
            printf("Error en timer start durante timer again\n");
            return 1;
        }
        printf("Timer start durante timer again\n");
    }

    return 0;
}

void
run_timer(uv_handle_t* handle){
    uv_timer_t* timer = (uv_timer_t*) handle;
    loopFSM_t* loop = timer->self.loop->loopFSM->user_data;

    printf("Tiempo proximo evento %d\n", (timer->timeout));
    printf("Tiempo actual %d\n", (loop->time));
    if(timer->timeout > loop->time)
        return;

    printf("Tiempo cumplido\n");
    uv_timer_again(timer);
    timer->timer_cb(timer);
}