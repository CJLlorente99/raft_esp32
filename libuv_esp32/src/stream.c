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

// virtual table for listen requests
static request_vtbl_t accept_req_vtbl = {
    .run = run_accept_req
};

int
uv_accept(uv_stream_t* server, uv_stream_t* client){
    uv_tcp_t* tcp = (uv_tcp_t*)server;
    int rv;
    
    uv_accept_t* req = malloc(sizeof(uv_accept_t));
    if(!req){
        ESP_LOGE("UV_ACCEPT", "Error during malloc in uv_accept");
        return 1;
    }

    req->req.loop = server->self.loop;
    req->req.vtbl = &accept_req_vtbl;
    req->server = server;
    req->client = client;

    rv = insert((void**)tcp->accept_requests,&(tcp->n_accept_requests),sizeof(uv_request_t*), req);
    if(rv != 0){
        ESP_LOGE("UV_ACCEPT","Error during insert in uv_accept");
        return 1;
    }

    return 0;
}

// virtual table for read_start_requests
static request_vtbl_t read_start_req_vtbl = {
    .run = run_read_start_req
};

int
uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb){
    ((uv_tcp_t*)stream)->alloc_cb = alloc_cb;
    ((uv_tcp_t*)stream)->read_cb = read_cb;
    int rv;

    uv_read_start_t* req = malloc(sizeof(uv_read_start_t));
    if(!req){
        ESP_LOGE("UV_READ_START", "Error during malloc in uv_read_start");
        return 1;
    }

    req->req.loop = stream->self.loop;
    req->req.vtbl = &read_start_req_vtbl;
    req->alloc_cb = alloc_cb;
    req->read_cb = read_cb;
    req->stream = stream;
    req->buf = NULL;
    req->nread = 0;
    req->is_alloc = 0;

    rv = insert((void**)stream->read_start_requests, &(stream->n_read_start_requests), sizeof(uv_request_t*), req);
    if(rv != 0){
        ESP_LOGE("UV_READ_START","Error during insert in uv_read_start");
        return 1;
    }

    return 0;
}

// virtual table for read_start_requests
static request_vtbl_t read_stop_req_vtbl = {
    .run = run_read_stop_req
};

int
uv_read_stop(uv_stream_t* stream){
    int rv;

    uv_read_stop_t* req = malloc(sizeof(uv_read_stop_t));
    if(!req){
        ESP_LOGE("UV_READ_START", "Error during malloc in uv_read_start");
        return 1;
    }

    req->req.loop = stream->self.loop;
    req->req.vtbl = &read_stop_req_vtbl;

    rv = insert((void**)stream->read_start_requests, &(stream->n_read_start_requests), sizeof(uv_request_t*), req);
    if(rv != 0){
        ESP_LOGE("UV_READ_START","Error during insert in uv_read_start");
        return 1;
    }

    return 0;
}

// virtual table for write_requests
static request_vtbl_t write_req_vtbl = {
    .run = run_write_req
};

int
uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb){
    int rv;
    uv_tcp_t* tcp = (uv_tcp_t*) handle;

    tcp->write_cb = cb;

    req->cb = cb;
    req->req.loop = handle->self.loop;
    req->req.vtbl = &write_req_vtbl;
    req->status = 0;
    req->bufs = bufs;
    req->nbufs = nbufs;

    rv = insert((void**)tcp->write_requests, &(tcp->n_write_requests), sizeof(uv_request_t*), req);
    if(rv != 0){
        ESP_LOGE("UV_WRITE","Error during insert in uv_write");
        return 1;
    }

    return 0;
}