#include "uv.h"


/* Init function. Initializes a socket */
int
uv_tcp_init(uv_loop_t* loop, uv_tcp_t* tcp){
    tcp->self.loop = loop;
    tcp->self.type = UV_TCP;
    tcp->self.vtbl = NULL;
    tcp->self.remove = 0;

    tcp->nreqs = 0;
    tcp->loop = loop;
    tcp->socket = -1;
    tcp->type = UV_TCP;

    /* Try to create socket */
    tcp->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(tcp->socket < 0){
        ESP_LOGE("UV_TCP_INIT", "Error during socket creation in uv_tcp_init");
        return 1;
    }

    /* Set non-blocking */
    uint32_t opt = fcntl(tcp->socket, F_GETFL,0);
    opt |= O_NONBLOCK;
    int ret = fcntl(tcp->socket, F_SETFL, opt);

    return 0;
}

/* Run function, vtbl and uv_tcp_bind */

/* Bind does not need from reactor*/
int
uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags){
    int rv;

    ESP_LOGI("bind","");
    rv = bind(handle->socket, addr, sizeof(struct sockaddr));
    if(rv != 0){
        ESP_LOGE("run_bind_handle", "Socket unable to bind in run_bind_handle: errno %d", errno);
        return 1;
    }

    return 0;
}

/* Run function, vtbl and uv_tcp_connect */

/* Run connect handle tries to connect previously created socket to a given sockaddr. If connection has succeeded cb will be called. Then removes handle  */
void
run_connect_handle(uv_handle_t* handle){
    int rv;
    uv_connect_t* connect_handle = (uv_connect_t*)handle;

    ESP_LOGI("run_connect_handle","");

    rv = connect(connect_handle->tcp->socket, &connect_handle->dest_sockaddr, sizeof(struct sockaddr));
    connect_handle->status = rv;
    if(rv != 0){
        if(errno != 120 && errno != 127){
            ESP_LOGE("run_connect_handle", "Error during connect in run_connect_handle: errno %d", errno);
            return;
        } else{
            connect_handle->status = 0;
        }
    }
        // TODO
        // No siempre que sea = -1 es error. Usar errno para saber que tipo de error ha sido y si debe volver a intentarse

    connect_handle->req.data = connect_handle->data;
    connect_handle->cb(connect_handle, connect_handle->status);

    rv = uv_remove_handle(connect_handle->loop->loop, handle);
    if(rv != 0){
        ESP_LOGE("run_connect_handle", "Unable to remove handle in run_connect_handle");
        return;
    }

    remove_req_from_stream((uv_stream_t*)connect_handle->tcp, handle);
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
    req->req.remove = 0;
    req->req.active = 1;

    req->cb = cb;
    memcpy(&req->dest_sockaddr, addr, sizeof(struct sockaddr));
    req->loop = handle->loop;
    req->status = 0;
    req->tcp = handle;
    req->type = UV_CONNECT;

    ESP_LOGI("connect","");

    add_req_to_stream((uv_stream_t*)handle, (uv_handle_t*)req);
    
    /* Add handle */
    rv = uv_insert_handle(handle->loop->loop, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_tcp_connect", "Error during uv_insert_request in uv_tcp_connect");
    }

    return 0;
}