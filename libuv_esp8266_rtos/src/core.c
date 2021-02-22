#include "uv.h"

#define LED_DEBUG_PORT 5

int
insert_handle(loopFSM_t* loop, uv_handle_t* handle){
    uv_handle_t** handlers = loop->active_handlers;
    int i = loop->n_active_handlers + 1; // array index

    // ERROR AQUI!!
    if(i == 1){
        handlers = malloc(sizeof(uv_handle_t*));
        // memcpy(*handlers, handle, sizeof(uv_handle_t*));
        *handlers = handle;
    } else{
        handlers = realloc(handlers, i * sizeof(uv_handle_t*));
        // memcpy(handlers[i-1], handle, sizeof(uv_handle_t*));
        handlers[i-1] = handle;
    }

    loop->n_active_handlers = i;

}

int
remove_handle(loopFSM_t* loop, uv_handle_t* handle){
    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_handlers - 1;
    uv_handle_t** new_handlers = malloc(new_n_active_handlers * sizeof(uv_handle_t*));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i] == (uv_handle_t*)handle){
            memcpy((uv_handle_t*)new_handlers[j++], loop->active_handlers[i], sizeof(uv_handle_t*));
            // new_handlers[j++] = loop->active_handlers[i];
        }
    }
}