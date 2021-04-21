#include "uv.h"

int
uv_insert_handle(loopFSM_t* loop, uv_handle_t* handle){
    
    /* Make sure this handler is new */
    for(int j = 0; j < loop->n_active_handlers; j++){
        if(loop->active_handlers[j] == handle)
            return 0;
    }

    loop->active_handlers[loop->n_active_handlers++] = handle;

    return 0;
}

int
uv_remove_handle(loopFSM_t* loop, uv_handle_t* handle){
    int j = 0;

    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i] != handle){
            loop->active_handlers[j++] = loop->active_handlers[i];
        }
    }

    loop->n_active_handlers--;
    free(handle);

    return 0;
}