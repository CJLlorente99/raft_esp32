#include "uv.h"


/* Init function. Initializes a socket */
int
uv_tcp_init(uv_loop_t* loop, uv_tcp_t* tcp){
    int rv;

    tcp->self.loop = loop;
    tcp->self.type = UV_TCP;
    tcp->self.vtbl = NULL;

    tcp->alloc_cb = NULL;
    tcp->close_cb = NULL;
    tcp->connect_cb = NULL;
    tcp->connection_cb = NULL;
    tcp->flags = 0;
    tcp->loop = loop;
    tcp->read_cb = NULL;
    tcp->server = tcp;
    tcp->socket = -1;
    tcp->src_sockaddr = NULL;
    tcp->type = UV_TCP;
    tcp->write_cb = NULL;

    /* Try to create socket */
    tcp->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(tcp->socket < 0){
        ESP_LOGE("UV_TCP_INIT", "Error during socket creation in uv_tcp_init");
        return 1;
    }

    // setsockopt()?

    return 0;
}

/* Run function, vtbl and uv_tcp_bind */

/* Run bind function just tries to bind previously initialized socket to a given sockaddr. Then removes handle. */
void
run_bind_handle(uv_handle_t* handle){
    int rv;
    uv_bind_t* bind_handle = (uv_bind_t*)handle;

    rv = bind(bind_handle->tcp->socket, bind_handle->tcp->src_sockaddr, sizeof(struct sockaddr));
    if(rv != 0){
        ESP_LOGE("run_bind_handle", "Socket unable to bind in run_bind_handle: errno %d", errno);
        return;
    }

    rv = uv_remove_handle(bind_handle->loop->loopFSM->user_data, handle);
    if(rv != 0){
        ESP_LOGE("run_bind_handle", "Unable to remove handle in run_bind_handle");
        return;
    }
} 

static handle_vtbl_t bind_handle_vtbl = {
    .run = run_bind_handle
};

int
uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags){
    int rv;

    handle->src_sockaddr = addr;

    uv_bind_t* req = malloc(sizeof(uv_bind_t));

    req->req.loop = handle->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &bind_handle_vtbl;

    req->loop = handle->loop;
    req->tcp = handle;
    
    /* Add handle */
    rv = uv_insert_handle(handle->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_tcp_bind", "Error during uv_insert_request in uv_tcp_bind");
    }

    return 0;
}

/* Run function, vtbl and uv_tcp_connect */

/* Run connect handle tries to connect previously created socket to a given sockaddr. If connection has succeeded cb will be called. Then removes handle  */
void
run_connect_handle(uv_handle_t* handle){
    int rv;
    uv_connect_t* connect_handle = (uv_connect_t*)handle;

    rv = connect(connect_handle->tcp->socket, connect_handle->dest_sockaddr, sizeof(struct sockaddr));
    connect_handle->status = rv;
    if(rv != 0){
        // TODO
        // No siempre que sea = -1 es error. Usar errno.h para saber que tipo de error ha sido y si debe volver a intentarse
        ESP_LOGE("run_connect_handle", "Error during connect in run_connect_handle: errno %d", errno);
        return; 
    }

    connect_handle->cb(connect_handle, connect_handle->status);

    rv = uv_remove_handle(connect_handle->loop->loopFSM->user_data, handle);
    if(rv != 0){
        ESP_LOGE("run_connect_handle", "Unable to remove handle in run_connect_handle");
        return;
    }
}

static handle_vtbl_t connect_handle_vtbl = {
    .run = run_connect_handle
};

int
uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct  sockaddr* addr, uv_connect_cb cb){
    int rv;

    req->req.loop = handle->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &connect_handle_vtbl;

    req->cb = cb;
    req->dest_sockaddr = addr;
    req->loop = handle->loop;
    req->status = 0;
    req->tcp = handle;
    req->type = UV_CONNECT;

    /* Add handle */
    rv = uv_insert_handle(handle->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_tcp_connect", "Error during uv_insert_request in uv_tcp_connect");
    }

    return 0;
}