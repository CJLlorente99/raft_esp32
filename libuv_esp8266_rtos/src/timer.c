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
    os_timer_t* new_timer;  

    handle->timer_cb = cb;
    handle->timeout = timeout;
    handle->repeat = repeat;
    handle->timer = new_timer;

    // Create timers
    // PROBLEMA CON xTimerCreate. SE DEBE INTRODUCIR CB DE TIPO TIMERCALLBACKFUNCTION_T.
    // COMO HAGO PARA QUE SE LLAME A HANDLE->TIMER_CB
    
    // repeat = 0 === one-shot?
    
    os_timer_setfn(handle->timer, (os_timer_func_t*)handle->timer_cb, (void*)handle);
    os_timer_arm(handle->timer, (uint32_t) handle->timeout, handle->repeat);
    // What is recommended os_timer_arm or xTimerCreate?

    // Timer should be started upon loop start

    return 0;
}

int
uv_timer_stop(uv_timer_t* handle){
    /*
    if(!uv__is_active(handle))
        return 0;
    */

    // Delete timer (xTimerDelete())
    os_timer_disarm(handle->timer);
}