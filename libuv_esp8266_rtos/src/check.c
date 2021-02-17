#include "uv.h"

// virtual table for check handlers
static handle_vtbl_t check_vtbl = {
    .run = run_check
};

int
uv_check_init(uv_loop_t* loop, uv_check_t* check){
    check->self->loop = loop;
    check->self->vtbl = &check_vtbl;
    return 0;
}

int
uv_check_start(uv_check_t* check, uv_check_cb cb){
    loopFSM_t* loop = check->self->loop->loopFSM->user_data;

    // TODO
    // comprobar si este handler ya esta presente en los que tienen que ser llamados. Si es asi, cambiar el cb

    // If not, add it to active handlers

    check->cb = cb;

    // TODO
    // añadir a los handlers a los que se tiene que llamar desde el loop

    return 0;   

}

int
uv_check_stop(uv_check_t* check){
    loopFSM_t* loop = check->self->loop->loopFSM->user_data;

    // quitar handler de los que son llamados desde el loop
}

// run implementation for signals
void
run_check(uv_handle_t* handle){
    uv_check_t* check = (uv_check_t*) handle;
    check->cb(check);
}