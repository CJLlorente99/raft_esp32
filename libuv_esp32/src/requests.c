#include "uv.h"

// run implementation for each request
void
run_connect_req(uv_request_t* req){
    uv_connect_t* connect_req = (uv_connect_t*) req;
    connect_req->cb(connect_req, connect_req->status);
}

void
run_listen_req(uv_request_t* req){
    uv_listen_t* listen_req = (uv_listen_t*) req;
    listen_req->cb(listen_req->stream, listen_req->status);
}