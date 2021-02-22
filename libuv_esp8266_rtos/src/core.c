#include "uv.h"

#define LED_DEBUG_PORT 5

int
insert_handle(loopFSM_t* loop, uv_handle_t* handle){
    uv_handle_t** handlers = loop->active_handlers;
    int i = loop->n_active_handlers + 1; // array index

    if(i == 1){
        handlers = malloc(sizeof(uv_handle_t*));
        memcpy(*handlers, handle, sizeof(uv_handle_t*));
    } else{
        handlers = realloc(handlers, i * sizeof(uv_handle_t*));
        // ERROR AQUI!!
        memcpy(handlers[i-1], handle, sizeof(uv_handle_t*));
    }

    loop->n_active_handlers = i;

}

int
remove_handle(loopFSM_t* loop, uv_handle_t* handle){
    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_handlers--;
    uv_handle_t** new_handlers = malloc(sizeof(uv_handle_t*[new_n_active_handlers]));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i] == (uv_handle_t*)handle){
            memcpy((uv_handle_t*)new_handlers[j++], loop->active_handlers[i], sizeof(uv_handle_t*));
        }
    }
}