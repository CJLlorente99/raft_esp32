#include "uv.h"

/* Run function, vtbl and uv_queue_work */
void
run_work_handle(uv_handle_t* handle){
    int rv;
    uv_work_t* work_handle = (uv_work_t*)handle;

    work_handle->work_cb(work_handle);
    work_handle->after_work_cb(work_handle, 0);

    rv = uv_remove_handle(work_handle->loop->loop, handle);
    if(rv != 0){
        ESP_LOGE("run_work_handle", "Error during uv_remove in run_work_handle");
        return;
    }
}

static handle_vtbl_t work_handle_vtbl = {
    .run = run_work_handle
};

int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb){
    int rv;
    
    req->req.loop = loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &work_handle_vtbl;
    
    req->after_work_cb = after_work_cb;
    req->loop = loop;
    req->type = UV_WORK;
    req->work_cb = work_cb;

    rv = uv_insert_handle(loop->loop, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_queue_work", "Error during uv_insert in uv_queue_work");
        return 1;
    }

    return 0;
}