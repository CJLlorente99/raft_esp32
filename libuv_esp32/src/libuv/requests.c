#include "uv.h"

// run implementation for each request
void
run_connect_req(uv_request_t* req){
    int rv;
    uv_connect_t* connect_req = (uv_connect_t*) req;
    loopFSM_t* loop = connect_req->loop->loopFSM->user_data;

    connect_req->cb(connect_req, connect_req->status);

    /* Requests are only called once */
    rv = uv_remove_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("RUN_CONNECT_REQ", "Error during uv_remove in run_connect_req");
        return;
    }
}

void
run_listen_req(uv_request_t* req){
    int rv;
    uv_listen_t* listen_req = (uv_listen_t*) req;
    loopFSM_t* loop = listen_req->loop->loopFSM->user_data;

    listen_req->cb(listen_req->stream, listen_req->status);

    rv = uv_remove_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("RUN_LISTEN_REQ", "Error during uv_remove in run_listen_req");
        return;
    }
}

void
run_accept_req(uv_request_t* req){
    int rv;
    uv_accept_t* accept_req = (uv_accept_t*) req;
    loopFSM_t* loop = accept_req->loop->loopFSM->user_data;

    rv = uv_remove_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("RUN_ACCEPT_REQ", "Error during uv_remove in run_accept_req");
        return;
    }
}

void
run_read_start_req(uv_request_t* req){
    uv_read_start_t* read_start_req = (uv_read_start_t*) req;
    if(read_start_req->alloc_cb){
        uv_buf_t* buf = malloc(sizeof(uv_buf_t));
        buf->len = 4*1024;

        read_start_req->buf = buf;
        read_start_req->alloc_cb((uv_handle_t*) read_start_req->stream, 4*1024, buf);
        read_start_req->is_alloc = 1;

        read_start_req->alloc_cb = NULL; // allocation should only be done once per read_start call
    }
    
    read_start_req->read_cb(read_start_req->stream, read_start_req->nread, read_start_req->buf);
    // if(read_start_req->nread == read_start_req->buf->len){
    //     rv = uv_remove_request(loop, (uv_request_t*)req);
    //     if(rv != 0){
    //         ESP_LOGE("RUN_READ_START_REQ", "Error during uv_remove in run_read_start_req");
    //         return;
    //     }
    // }

    
}

void
run_read_stop_req(uv_request_t* req){
    int rv;
    uv_read_stop_t* read_stop_req = (uv_read_stop_t*) req;
    loopFSM_t* loop = read_stop_req->loop->loopFSM->user_data;

    rv = uv_remove_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("RUN_READ_STOP_REQ", "Error during uv_remove in run_read_stop_req");
        return;
    }

    rv = uv_remove_request(loop, read_stop_req->read_start_req);
    if(rv != 0){
        ESP_LOGE("RUN_READ_STOP_REQ", "Error during uv_remove in run_read_stop_req");
        return;
    }
}

void
run_write_req(uv_request_t* req){
    int rv;
    uv_write_t* write_req = (uv_write_t*) req;
    loopFSM_t* loop = write_req->stream->server->loop->loopFSM->user_data;

    write_req->cb(write_req, write_req->status);

    rv = uv_remove_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("RUN_WRITE_REQ", "Error during uv_remove in run_write_req");
        return;
    }
}

void
run_fs_req(uv_request_t* req){
    // TODO (never used in raft)
}

void
run_work_req(uv_request_t* req){
    int rv;
    uv_work_t* work_req = (uv_work_t*) req;
    loopFSM_t* loop = work_req->loop->loopFSM->user_data;

    work_req->work_cb(work_req);
    work_req->after_work_cb(work_req, 0);

    rv = uv_remove_request(loop, (uv_request_t*)req);
    if(rv != 0){
        ESP_LOGE("RUN_WRITE_REQ", "Error during uv_remove in run_write_req");
        return;
    }
}