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
    // no hace falta crear una funcion como tal ya que uv__stream_init solo se usa aqui
    tcp->self = malloc(sizeof(uv_handle_t));
    tcp->self->loop = loop_s;
    tcp->self->vtbl = &tcp_vtbl;

    loopFSM_t* loop = loop_s->loopFSM->user_data;

    esp_tcp* esp_tcp_s = malloc(sizeof(esp_tcp));
    tcp->espconn_s->proto.tcp = esp_tcp_s;

    tcp->alloc_cb = NULL;
    tcp->close_cb = NULL;
    tcp->connection_cb = NULL;
    tcp->read_cb = NULL;

    tcp->n_connect_requests = 0;
    tcp->connect_requests = NULL;

    // añadir a los handlers a los que se tiene que llamar desde el loop
    insert_handle(loop, (uv_handle_t*)tcp);
}

// int
// uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags){
//     unsigned int addrlen;

//     // Can only be used in ipv4 version
//     // this function creates a socket (AF_INET4) and stores it inside handle as a fd
//     // sets sockets options (SOL_SOCKET, SO_REUSEADDR, etc)
//     // finally binds the socket to the IPv4 address

//     // PROBLEMS -> sockaddr exists in esp8266 rtos?
    
//     // TODO
//     // take address from struct sockaddr
//     // Understanding how espconn works, this function should only safe the information given in the arguments

//     int rv = 0;

//     if(rv != 0){
//         // do something because error might have ocurred
//     }


// }

// int
// uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct  sockaddr* addr, uv_connect_cb cb){
//     // this function connects the socket in handle to addr in sockeaddr
//     // once connection is completed callback is triggered (added to the correspondant state in loop FSM to be executed once)
//     // this connection cb is a handshake or similar. That why a pollout whatcher is needed to send handshake msg
    
//     // Use espconn_connect
//     // it is recommended to use espconn_port to get an available local port
//     // espconn_recv_hold?


//     handle->connect_cb = cb;

//     int rv = 0;

//     // rv = handle->espconn_s->proto.tcp->remote_ip = addr->sa_data;
//     if(rv != 0){
//         // do something because error might have ocurred
//     }

//     handle->n_connect_requests++;

//     handle->connect_requests = realloc(handle->connect_requests, (handle->n_connect_requests)* sizeof(uv_connect_t*));
//     handle->connect_requests[handle->n_connect_requests - 1] = req;
//     // memcpy(handle->connect_requests[handle->n_connect_requests - 1], &req, sizeof(uv_connect_t*))

// }

// run implementation for tcps
void
run_tcp(uv_handle_t* handle){
    // TODO
    uv_tcp_t* tcp = (uv_tcp_t*) handle;
    if(tcp->n_connect_requests > 0){
        int j = 0;
        for(int i = 0; i < tcp->n_connect_requests; i++){
            if(espconn_connect(tcp->espconn_s)){
                int status = 0; // lo que devuelve espconn_connect
                tcp->connect_cb(tcp->connect_requests[i], status);

                j++;
            }
            // TODO
            // dejar los request no atendidos, quitar los atendidos
        }
        tcp->n_connect_requests -= j;
    }

    if(tcp->n_accept_requests > 0){
        if(espconn_accept(tcp->espconn_s)){
            espconn_recv_hold(tcp->espconn_s);
            tcp->n_accept_requests = 0;

        }
    }

    if(tcp->n_listen_requests > 0){
        int status = 0;
        tcp->connection_cb((uv_stream_t*)handle, status);
    }
}