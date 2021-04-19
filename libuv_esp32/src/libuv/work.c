#include "uv.h"

// virtual table for work requests
static request_vtbl_t work_req_vtbl = {
    .run = run_work_req
};

int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb){
    req->type = UV_WORK;
    req->loop = loop;
    req->work_cb = work_cb;
    req->after_work_cb = after_work_cb;
    req->req.vtbl = &work_req_vtbl;

    int rv;

    rv = uv_insert_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_queue_work", "Error during uv_insert in uv_queue_work");
        return 1;
    }

    return 0;
}