#include "uv.h"

int
uv_timer_init (uv_loop_t* loop, uv_timer_t* handle){
    handle->loop = loop;
    return 0;
}

int
uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;

    os_timer_t* new_timer;  

    handle->timer_cb = cb;
    handle->timeout = timeout;
    handle->repeat = repeat;
    handle->timer = new_timer;

    // Create timers
    // PROBLEMA CON xTimerCreate. SE DEBE INTRODUCIR CB DE TIPO TIMERCALLBACKFUNCTION_T.
    // COMO HAGO PARA QUE SE LLAME A HANDLE->TIMER_CB
    
    // 0 => periodic; 1 => one-shot
    
    os_timer_setfn(handle->timer, (os_timer_func_t*)handle->timer_cb, (void*)handle);
    os_timer_arm(handle->timer, (uint32_t) handle->timeout, !(handle->repeat));
    // What is recommended os_timer_arm or xTimerCreate?

    // Timer should be started upon loop start

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    // Delete timer (xTimerDelete())
    os_timer_disarm(handle->timer);

    return 0;
}