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

    tcp->self.loop = loop_s;
    tcp->self.vtbl = &tcp_vtbl;

    loopFSM_t* loop = loop_s->loopFSM->user_data;

    tcp->alloc_cb = NULL;
    tcp->close_cb = NULL;
    tcp->connection_cb = NULL;
    tcp->read_cb = NULL;

    tcp->n_connect_requests = 0;
    tcp->connect_requests = NULL;

    // Create socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(tcp_socket < 0){
        ESP_LOGE("UV_TCP_INIT", "Error during socket creation in uv_tcp_init");
        return 1;
    }

    tcp->socket = tcp_socket;

    // añadir a los handlers a los que se tiene que llamar desde el loop
    rv = insert_handle(loop, (uv_handle_t*)tcp);
    if(rv != 0){
        ESP_LOGE("UV_TCP_INIT", "Error during insert handle in uv_tcp_init");
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

int
uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct  sockaddr* addr, uv_connect_cb cb){
    // this function connects the socket in handle to addr in sockeaddr
    // once connection is completed callback is triggered (added to the correspondant state in loop FSM to be executed once)
    // this connection cb is a handshake or similar. That why a pollout whatcher is needed to send handshake msg
    int rv;

    req->req.loop = handle->self.loop;
    req->cb = cb;
    req->dest_sockaddr = addr;

    handle->n_connect_requests++;
    handle->connect_requests = realloc(handle->connect_requests, handle->n_connect_requests * sizeof(uv_connect_t*));
    if(!handle->connect_requests){
        ESP_LOGE("UV_TCP_CONNECT", "Error during realloc in uv_tcp_connect");
        return 1;
    }
    memcpy(&(handle->connect_requests[handle->n_connect_requests-1]), &req, sizeof(uv_connect_t*));

    /* Add request to the request list */
    rv = insert_request(req, handle->self.loop);
    if(rv != 0){
        ESP_LOGE("UV_TCP_CONNECT", "Error during insert_request in uv_tcp_connect");
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
    int rv;

    if(tcp->bind){
        rv = bind(tcp->socket, tcp->src_sockaddr, sizeof(struct sockaddr));
        if(rv != 0){
            ESP_LOGE("RUN_TCP", "Socket unable to bind in run tcp");
            return;
        }
        tcp->bind = 0;
    }

    if(tcp->n_connect_requests > 0){ // connect es bloqueante?
        for(int i = 0; i < tcp->n_connect_requests; i++){
            rv = connect(tcp->socket, tcp->connect_requests[i]->dest_sockaddr, sizeof(struct sockaddr));
            if(rv != 0){
                ESP_LOGE("RUN_TCP", "Error during connect in run tcp");
                return;
            }
            tcp->n_connect_requests--;
            // add request to be called in following state (run_requests)
            // take out request from tcp object
        }
    }

    if(tcp->n_listen_requests > 0){
        for(int i = 0; i < tcp->n_listen_requests; i++){
            rv = listen(tcp->socket, 1);
            if(rv != 0){
                ESP_LOGE("RUN_TCP", "Error durign listen in run tcp");
                return;
            }
            tcp->n_listen_requests--;
            // add request to be called in following state (run_requests)
            // take out  request from tcp object
        }
    }

    if(tcp->n_accept_requests > 0){
        for(int i = 0; i < tcp->n_listen_requests; i++){
            rv = accept(tcp->socket, tcp->accept_requests[i]->client->src_sockaddr, sizeof(struct sockaddr));
            if(rv != 0){
                ESP_LOGE("RUN_TCP", "Error during accept in run tcp\n");
                return;
            }
            tcp->n_accept_requests--;
            // take out  request from tcp object
        }
    }
}