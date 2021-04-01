#include "uv.h"

/*
    usar librería FreeRTOS+TCP?

    En vez de centrarme tanto en como se hace en el libuv original. Sería posible
    hacerlo con una estructura reactor.

    uv_tcp
        tcp_init -> inicializa el reactor (se llama desde el loop base)
        tcp_close -> quitar el reactor del loop base
        tcp_bind -> crea un socket y lo une a una direccion (esta info se añade en el objeto tcp)
        tcp_connect -> conecta el socket con una direccion (añadir conexiones a un objeto tcp)

    uv_stream
        listen -> habilitar la escucha de nuevas conexiones entrantes (habilita que el reactor mire una conexion). Se guardan "backlog" conexiones
        accept -> acepta las conexiones de listen (habilita que el rector comience a aceptar)
        read_start -> comienza a leer hasta que no hay más que leer o read_stop es llamado (habilita al reactor a leer)
        read_stop -> parar de leer (deshabilita la lectura)
        write -> escribir
*/

// Even though espconn has various function to attach cb to some events, this would imply asyncronous beahviour.
// Therefore cb calls will be managed differently

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

    if(tcp->bind){
        rv = bind(tcp->socket, tcp->src_sockaddr, sizeof(struct sockaddr));
        if(rv != 0){
            ESP_LOGE("RUN_TCP_BIND", "Socket unable to bind in run tcp: errno %d", errno);
            // do we close() socket if error occurs?
            return;
        }
        tcp->bind = 0;
    }

    if(tcp->n_connect_requests > 0){ // connect es bloqueante?
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
    }

    if(tcp->n_listen_requests > 0){
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
    }

    // accept es bloqueante, ver el fd y ver si hay algo que leer
    if(tcp->n_accept_requests > 0){ 
        for(int i = 0; i < tcp->n_accept_requests; i++){
            uv_accept_t* req;
            memcpy(&req, (uv_accept_t**)&tcp->accept_requests[i], sizeof(uv_accept_t*));
            // select 
            if(select(tcp->socket, &(tcp->readset), NULL, NULL, NULL)){
                char addr[16];
                inet_ntop(AF_INET, &(((struct sockaddr_in *)req->client->src_sockaddr)->sin_addr),
                    addr, 16);
                ESP_LOGI("RUN_TCP_ACCEPT", "accepting : %s", addr);
                socklen_t size = sizeof(struct sockaddr);
                rv = accept(tcp->socket, req->client->src_sockaddr, &size);
                if(rv != 0){
                    ESP_LOGE("RUN_TCP_ACCEPT", "Error during accept in run tcp: errno %d", errno);
                    return;
                }
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
    }

    // not sure this functionality is what it actually should be
    // I think read should be called until buffer is full
    if(tcp->n_read_start_requests > 0){ 
        for(int i = 0; i < tcp->n_read_start_requests; i++){
            uv_read_start_t* req;
            memcpy(&req, (uv_read_start_t**)&tcp->read_start_requests[i], sizeof(uv_read_start_t*));
            if(select(tcp->socket, &(tcp->readset), NULL, NULL, NULL) && req->is_alloc){
                // no se debería utilizar nread para saber cuanto falta por completar del buffer
                // req->buf->base + nread, req->buf->len - nread
                ssize_t nread = read(tcp->socket, req->buf->base, req->buf->len);
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
    }

    if(tcp->n_read_stop_requests > 0){
        for(int i = 0; i < tcp->n_read_stop_requests; i++){
            rv = uv_remove_tcp(tcp, tcp->read_start_requests[i], READ_START);
            if(rv != 0){
                ESP_LOGE("RUN_TCP_READ_STOP", "Error during uv_remove in run_tcp");
                return;
            }

            rv = uv_remove_tcp(tcp, tcp->read_stop_requests[i], READ_STOP);
            if(rv != 0){
                ESP_LOGE("RUN_TCP_READ_STOP", "Error during uv_remove in run_tcp");
                return;
            }
        }
    }

    if(tcp->n_write_requests > 0){ 
        for(int i = 0; i < tcp->n_write_requests; i++){
            uv_write_t* req;
            memcpy(&req, (uv_write_t**)&tcp->write_requests[i], sizeof(uv_write_t*));
            if(select(tcp->socket, NULL, &(tcp->writeset), NULL, NULL)){
                rv = write(tcp->socket, req->bufs, req->nbufs * sizeof(req->bufs[0]));
                req->status = rv;
                if(rv < 0){
                    ESP_LOGE("RUN_TCP_WRITE", "Error during write in run_tcp: errno %d", errno);
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

}