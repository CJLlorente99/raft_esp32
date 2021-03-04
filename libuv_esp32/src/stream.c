#include "uv.h"

// virtual table for listen requests
static request_vtbl_t listen_req_vtbl = {
    .run = run_listen_req
};

int
uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb){
    // This function should only safe information given through the arguments
    // backlog indicates number of connection to be queued (only backlog = 1 is used in raft)
    uv_tcp_t* tcp = (uv_tcp_t*)stream;
    int rv;

    tcp->connection_cb = cb;

    uv_listen_t* req = malloc(sizeof(uv_listen_t));
    if(!req){
        ESP_LOGE("UV_LISTEN", "Error during malloc in uv_listen");
        return 1;
    }

    req->req.loop = tcp->self.loop;
    req->req.vtbl = &listen_req_vtbl;
    req->cb = cb;
    req->stream = stream;
    req->status = 0;

    // tcp->n_listen_requests++;
    // tcp->listen_requests = realloc(tcp->listen_requests, tcp->n_listen_requests * sizeof(uv_listen_t*));
    // if(!tcp->listen_requests){
    //     printf("UV_LISTEN", "Error during realloc in uv_listen");
    //     return 1;
    // }
    // memcpy(&(tcp->listen_requests[tcp->n_listen_requests-1]), &req, sizeof(uv_listen_t*));

    rv = insert((void**)tcp->listen_requests,&(tcp->n_listen_requests),sizeof(uv_request_t*), req);
    if(rv != 0){
        ESP_LOGE("UV_LISTEN","Error during insert in uv_listen");
        return 1;
    }
        
    return 0;
}

int
uv_accept(uv_stream_t* server, uv_stream_t* client){
    uv_tcp_t* tcp = (uv_tcp_t*)server;
    
    uv_accept_t* req = malloc(sizeof(uv_accept_t));
    if(!req){
        ESP_LOGE("UV_ACCEPT", "Error during malloc in uv_accept");
        return 1;
    }

    req->server = server;
    req->client = client;

    tcp->n_accept_requests++;
    tcp->accept_requests = realloc(tcp->accept_requests, tcp->n_accept_requests * sizeof(uv_accept_t*));
    if(!tcp->accept_requests){
        ESP_LOGE("UV_ACCEPT", "Error during realloc in uv_accept");
        return 1;
    }
    memcpy(&(tcp->accept_requests[tcp->n_accept_requests-1]), &req, sizeof(uv_listen_t*));

    return 0;
}

int
uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb){
    ((uv_tcp_t*)stream)->alloc_cb = alloc_cb;
    ((uv_tcp_t*)stream)->read_cb = read_cb;

    return 0;
}

int
uv_read_stop(uv_stream_t* stream){
    return 0;
}

int
uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb){
    ((uv_tcp_t*)handle)->write_cb = cb;
    return 0;
}