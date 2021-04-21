#include "uv.h"

void run_tcp(uv_handle_t* handle);

// virtual table for tcp handlers
static handle_vtbl_t tcp_vtbl = {
    .run = run_tcp
};

int
uv_tcp_init(uv_loop_t* loop_s, uv_tcp_t* tcp){
    int rv;

    tcp->loop = loop_s;
    tcp->self.vtbl = &tcp_vtbl;

    loopFSM_t* loop = loop_s->loopFSM->user_data;

    tcp->type = UV_TCP;
    tcp->flags = 0;
    tcp->listen_requests = NULL;
    tcp->n_listen_requests = 0;
    tcp->accept_requests = NULL;
    tcp->n_accept_requests = 0;
    tcp->connect_requests = NULL;
    tcp->n_connect_requests = 0;
    tcp->read_start_requests = NULL;
    tcp->n_read_start_requests = 0;
    tcp->read_stop_requests = NULL;
    tcp->n_read_stop_requests = 0;
    tcp->write_requests = NULL;
    tcp->n_write_requests = 0;
    tcp->bind = 0;
    tcp->alloc_cb = NULL;
    tcp->close_cb = NULL;
    tcp->connection_cb = NULL;
    tcp->read_cb = NULL;
    FD_ZERO(&(tcp->readset));
    FD_ZERO(&(tcp->writeset));
    FD_ZERO(&(tcp->errorset));

    // Create socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(tcp_socket < 0){
        ESP_LOGE("UV_TCP_INIT", "Error during socket creation in uv_tcp_init");
        return 1;
    }

    // setsockopt()?

    tcp->socket = tcp_socket;
    FD_SET(tcp_socket, &(tcp->readset));
    FD_SET(tcp_socket, &(tcp->writeset));
    FD_SET(tcp_socket, &(tcp->errorset));

    // añadir a los handlers a los que se tiene que llamar desde el loop
    rv = uv_insert_handle(loop, (uv_handle_t*)tcp);
    if(rv != 0){
        ESP_LOGE("UV_TCP_INIT", "Error during uv_insert handle in uv_tcp_init: errno %d", errno);
        return 1;
    }

    return 0;
}

int
uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags){
    // TODO
    // in raft implementation flags is always = 0 => do nothing with flags

    handle->bind = 1;
    handle->src_sockaddr = addr;
    return 0;
}

// virtual table for connect requests
static request_vtbl_t connect_req_vtbl = {
    .run = run_connect_req
};

int
uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct  sockaddr* addr, uv_connect_cb cb){
    // this function connects the socket in handle to addr in sockeaddr
    // once connection is completed callback is triggered (added to the correspondant state in loop FSM to be executed once)
    // this connection cb is a handshake or similar. That why a pollout whatcher is needed to send handshake msg
    // loopFSM_t* loop = handle->self.loop->loopFSM->user_data;
    int rv;

    req->req.vtbl = &connect_req_vtbl;
    req->loop = handle->loop;
    req->cb = cb;
    req->dest_sockaddr = addr;
    req->status = 0;
    req->type = UV_CONNECT;

    /* Add request to the request list */
    rv = uv_insert_tcp(handle, (uv_request_t*)req, CONNECT);
    if(rv != 0){
        ESP_LOGE("UV_TCP_CONNECT", "Error during uv_insert_request in uv_tcp_connect");
    }

    return 0;
}

// run implementation for tcps
void
run_tcp(uv_handle_t* handle){
    // TODO
    // this function checks if any tcp/stream function has been called and tries to execute it
    // if it is executed, creates a request for the function callback
    uv_tcp_t* tcp = (uv_tcp_t*)handle;
    loopFSM_t* loop = tcp->loop->loopFSM->user_data;
    int rv;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // Refresh fd
    FD_ZERO(&(tcp->readset));
    FD_ZERO(&(tcp->writeset));
    FD_ZERO(&(tcp->errorset));
    FD_SET(tcp->socket, &(tcp->readset));
    FD_SET(tcp->socket, &(tcp->writeset));
    FD_SET(tcp->socket, &(tcp->errorset));

    if(tcp->bind){
        rv = bind(tcp->socket, tcp->src_sockaddr, sizeof(struct sockaddr));
        if(rv != 0){
            ESP_LOGE("RUN_TCP_BIND", "Socket unable to bind in run tcp: errno %d", errno);
            // do we close() socket if error occurs?
            return;
        }
        tcp->bind = 0;
    }

    for(int i = 0; i < tcp->n_connect_requests; i++){
        uv_connect_t* req = (uv_connect_t*)tcp->connect_requests[i];
        rv = connect(tcp->socket, req->dest_sockaddr, sizeof(struct sockaddr));
        req->status = rv;
        if(rv != 0){
            // TODO
            // No siempre que sea = -1 es error. Usar errno.h para saber que tipo de error ha sido y si debe volver a intentarse
            ESP_LOGE("RUN_TCP_CONNECT", "Error during connect in run tcp: errno %d", errno);
            return; 
        }
        // add request to be called in following state (run_requests)
        // take out request from tcp object
        rv = uv_insert_request(loop, tcp->connect_requests[i]);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_CONNECT", "Error during uv_insert in run_tcp");
            return;
        }
        rv = uv_remove_tcp(tcp, tcp->connect_requests[i], CONNECT);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_CONNECT", "Error during uv_remove in run_tcp");
            return;
        }
    }

    for(int i = 0; i < tcp->n_listen_requests; i++){
        uv_listen_t* req;
        memcpy(&req,(uv_listen_t**)&tcp->listen_requests[i], sizeof(uv_listen_t*));
        rv = listen(tcp->socket, 1);
        req->status = rv;
        if(rv != 0){
            ESP_LOGE("RUN_TCP_LISTEN", "Error durign listen in run tcp: errno %d", errno);
            return;
        }
        // add request to be called in following state (run_requests)
        // take out  request from tcp object
        rv = uv_insert_request(loop, tcp->listen_requests[i]);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_LISTEN", "Error during uv_insert in run_tcp");
            return;
        }
        rv = uv_remove_tcp(tcp, tcp->listen_requests[i], LISTEN);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_LISTEN", "Error during uv_remove in run_tcp");
            return;
        }
    }

    // accept es bloqueante, ver el fd y ver si hay algo que leer
    for(int i = 0; i < tcp->n_accept_requests; i++){
        uv_accept_t* req;
        memcpy(&req, (uv_accept_t**)&tcp->accept_requests[i], sizeof(uv_accept_t*));
        // select 
        FD_ZERO(&(tcp->readset));
        FD_SET(tcp->socket, &(tcp->readset));

        if(select(tcp->socket + 1, &(tcp->readset), NULL, NULL, &tv)){
            rv = accept(tcp->socket, NULL, NULL);
            if(rv == -1){
                ESP_LOGE("RUN_TCP_ACCEPT", "Error during accept in run tcp: errno %d", errno);
                return;
            }
            char addr;
            struct sockaddr name;
            socklen_t len;
            getpeername(rv,&name,&len);
            inet_ntop(AF_INET, &(((struct sockaddr_in*)&name)->sin_addr), &addr, len);
            ESP_LOGI("RUN_TCP_ACCEPT", "accepting : %s", &addr);
            req->client->socket = rv;
            // take out request from tcp object
            rv = uv_insert_request(loop, tcp->accept_requests[i]);
            if(rv != 0){
                ESP_LOGE("RUN_TCP_ACCEPT", "Error during uv_insert in run_tcp");
                return;
            }
            rv = uv_remove_tcp(tcp, tcp->accept_requests[i], ACCEPT);
            if(rv != 0){
                ESP_LOGE("RUN_TCP_ACCEPT", "Error during uv_remove in run_tcp");
                return;
            }
        }
    }

    for(int i = 0; i < tcp->n_read_start_requests; i++){
        uv_read_start_t* req;
        memcpy(&req, (uv_read_start_t**)&tcp->read_start_requests[i], sizeof(uv_read_start_t*));
        FD_ZERO(&(tcp->readset));
        FD_SET(tcp->socket, &(tcp->readset));

        if(select(tcp->socket + 1, &(tcp->readset), NULL, NULL, &tv) && req->is_alloc){
            // CUIDADO, raft aumenta el mismo buf.base y reduce buf.len
            // ssize_t nread = read(tcp->socket, req->buf->base, req->buf->len);
            // req->nread = nread;
            ssize_t nread = read(tcp->socket, req->buf->base + req->nread, req->buf->len - req->nread);
            req->nread += nread;
            if(nread < 0){
                ESP_LOGE("RUN_TCP_READ_START", "Error during read in run_tcp: errno %d", errno);
            }
        }
        rv = uv_insert_request(loop, (uv_request_t*)req);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_READ_START", "Error during uv_insert in run_tcp");
            return;
        }
        // DO NOT uv_remove the read start request (uv_read_stop is the one doing that)
    }

    for(int i = 0; i < tcp->n_read_stop_requests; i++){
        uv_read_stop_t* req;
        memcpy(&req, (uv_read_stop_t**)&tcp->read_stop_requests[i], sizeof(uv_read_stop_t*));

        /* Stop the correspoding read_start_request */
        for(int j = 0; j < tcp->n_read_start_requests; j++){
            if(((uv_read_start_t*)tcp->read_start_requests[j])->stream == req->stream){
                req->read_start_req = tcp->read_start_requests[j];
                rv = uv_remove_tcp(tcp, tcp->read_start_requests[j], READ_START);
                if(rv != 0){
                    ESP_LOGE("RUN_TCP_READ_STOP", "Error during uv_remove in run_tcp");
                    return;
                }
            }
        }
        
        rv = uv_remove_tcp(tcp, tcp->read_stop_requests[i], READ_STOP);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_READ_STOP", "Error during uv_remove in run_tcp");
            return;
        }

        rv = uv_insert_request(loop, (uv_request_t*)req);
        if(rv != 0){
            ESP_LOGE("RUN_TCP_READ_STOP", "Error during uv_insert in run_tcp");
            return;
        }
    }

    for(int i = 0; i < tcp->n_write_requests; i++){
        uv_write_t* req;
        memcpy(&req, (uv_write_t**)&tcp->write_requests[i], sizeof(uv_write_t*));
        uv_stream_t* client = req->stream;

        FD_ZERO(&(client->writeset));
        FD_SET(client->socket, &(client->writeset));
        if(select(client->socket + 1, NULL, &(client->writeset), NULL, &tv)){
            ESP_LOGI("RUN_TCP_WRITE", "%s %d", req->bufs->base, req->bufs->len);
            rv = write(client->socket, req->bufs->base, req->nbufs * req->bufs->len);
            ESP_LOGI("RUN_TCP_WRITE", "Written");
            req->status = rv;
            if(rv < 0){
                ESP_LOGE("RUN_TCP_WRITE", "Error during write in run_tcp: errno %d", errno);
            }

            if(rv != req->nbufs * req->bufs->len){
                /*  Should be called again in order to write everything */
                req->bufs->base += rv;
                req->bufs->len = req->nbufs * req->bufs->len - rv;
                req->nbufs = 1;
                return;
            }

            rv = uv_insert_request(loop, tcp->write_requests[i]);
            if(rv != 0){
                ESP_LOGE("RUN_TCP_WRITE", "Error during uv_insert in run_tcp");
                return;
            }

            rv = uv_remove_tcp(tcp, tcp->write_requests[i], WRITE);
            if(rv != 0){
                ESP_LOGE("RUN_TCP_WRITE", "Error during uv_remove in run_tcp");
                return;
            }
        }
    }

}