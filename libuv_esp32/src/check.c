#include "uv.h"

// run implementation for signals
void
run_check(uv_handle_t* handle){
    uv_check_t* check = (uv_check_t*) handle;
    check->cb(check);
}

// virtual table for check handlers
static handle_vtbl_t check_vtbl = {
    .run = run_check
};

int
uv_check_init(uv_loop_t* loop, uv_check_t* check){
    check->self.loop = loop;
    check->self.vtbl = &check_vtbl;

    check->cb = NULL;

    return 0;
}

int
uv_check_start(uv_check_t* handle, uv_check_cb cb){
    loopFSM_t* loop = handle->self.loop->loopFSM->user_data;
    int rv;

    // Check if handle is already present, if it is, just change cb
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i] == (uv_handle_t*)handle){
            handle->cb = cb;
            return 0;
        }
    }

    // If not, add it to active handlers
    handle->cb = cb;

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        printf("Error durante uv_insert handle en check start\n");
        return 1;
    }

    return 0;   

}

int
uv_check_stop(uv_check_t* handle){
    loopFSM_t* loop = handle->self.loop->loopFSM->user_data;
    int rv;

    rv = uv_remove_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        printf("Error durante uv_remove handle en check stop\n");
        return 1;
    }

    return 0;
}