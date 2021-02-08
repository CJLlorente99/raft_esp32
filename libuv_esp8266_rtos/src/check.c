#include "uv.h"

int
uv_check_init(uv_loop_t* loop, uv_check_t* check){
    check->loop = loop;
    return 0;
}

int
uv_check_start(uv_check_t* check, uv_check_cb cb){
    loopFSM_t* loop = check->loop->loopFSM->user_data;

    // check if this check is not present in actual loop active handlers. If it is, just change cb
    if(loop->active_check_handlers){
    // Check if handler with same signum
        uv_check_t** handlers = loop->active_check_handlers;
        for(int i = 0; i < loop->n_active_check_handlers; i++){
            if(handlers[i] == check){
                check->cb = cb;
                return 0;
            }
        }
    }

    // If not, add it to active handlers

    check->cb = cb;

    uv_check_t** handlers = loop->active_check_handlers;
    int i = loop->n_active_check_handlers; // array index

    if(loop->n_active_check_handlers == 0){
        *handlers = malloc(sizeof(uv_check_t));
        memcpy((uv_check_t*)handlers[0], check, sizeof(uv_check_t));
    } else {
        *handlers = realloc(*handlers, sizeof(uv_check_t[i]));
        memcpy((uv_check_t*)handlers[i], check, sizeof(uv_check_t));
    }

    loop->n_active_check_handlers++;

    return 0;   

}

int
uv_check_stop(uv_check_t* check){
    loopFSM_t* loop = check->loop->loopFSM->user_data;

    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_check_handlers--;
    uv_check_t** new_handlers = malloc(sizeof(uv_check_t[new_n_active_handlers]));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_check_handlers; i++){
        if(loop->active_check_handlers[i] != check){
            memcpy((uv_check_t*)new_handlers[j], loop->active_check_handlers[i], sizeof(uv_check_t));
            j++;
        }
    }

    // Exchange in loop structure
    loop->active_check_handlers = new_handlers;
}