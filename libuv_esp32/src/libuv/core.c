#include "uv.h"

int
uv_insert_handle(loopFSM_t* loop, uv_handle_t* handle){
    
    /* Make sure this handler is new */
    // for(int j = 0; j < loop->n_active_handlers; j++){
    //     if(loop->active_handlers[j] == handle)
    //         return 0;
    // }

    loop->active_handlers[loop->n_active_handlers++] = handle;

    return 0;
}

int
uv_remove_handle(loopFSM_t* loop, uv_handle_t* handle){

    if((handle->type == UV_TCP) || (handle->type == UV_STREAM)){
        uv_stream_t* stream = (uv_stream_t*)handle;
        for(int i = 0; i < stream->nreqs; i++){
            stream->reqs[i]->remove = 1;
        }
    }

    handle->remove = 1;

    return 0;
}

void
add_req_to_stream(uv_stream_t* stream, uv_handle_t* req){
    stream->reqs[stream->nreqs++] = req;
}