#include "uv.h"

int
uv_check_init(uv_loop_t* loop, uv_check_t* check){
    check->loop = loop;
    check->self->type = CHECK;
    check->self->handle_check = check;
    return 0;
}

int
uv_check_start(uv_check_t* check, uv_check_cb cb){
    loopFSM_t* loop = check->loop->loopFSM->user_data;

    // check if this check is not present in actual loop active handlers. If it is, just change cb
    if(loop->active_handlers){
        uv_handle_t** handlers = loop->active_handlers;
        for(int i = 0; i < loop->n_active_handlers; i++){
            if(handlers[i]->type == CHECK){
                if(handlers[i]->handle_check == check){
                    check->cb = cb;
                    return 0;
                }
            }
        }
    }

    // If not, add it to active handlers

    check->cb = cb;

    uv_handle_t** handlers = loop->active_handlers;
    int i = loop->n_active_handlers; // array index

    if(loop->n_active_handlers == 0){
        *handlers = malloc(sizeof(uv_handle_t));
        memcpy((uv_handle_t*)handlers[0], check->self, sizeof(uv_handle_t));
    } else {
        *handlers = realloc(*handlers, sizeof(uv_handle_t[i]));
        memcpy((uv_handle_t*)handlers[i], check->self, sizeof(uv_handle_t));
    }

    loop->n_active_handlers++;

    return 0;   

}

int
uv_check_stop(uv_check_t* check){
    loopFSM_t* loop = check->loop->loopFSM->user_data;

    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_handlers--;
    uv_handle_t** new_handlers = malloc(sizeof(uv_handle_t[new_n_active_handlers]));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->type == CHECK){
            if(loop->active_handlers[i]->handle_check != check){
                memcpy((uv_handle_t*)new_handlers[j++], loop->active_handlers[i], sizeof(uv_handle_t));
            }
        }
    }

    // Exchange in loop structure
    loop->active_handlers = new_handlers;
}