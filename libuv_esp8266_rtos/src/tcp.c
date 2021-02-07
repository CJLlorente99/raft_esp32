#include "uv.h"

/*
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

int
uv_tcp_init(uv_loop_t* loop_s, uv_tcp_t* tcp){
    // no hace falta crear una funcion como tal ya que uv__stream_init solo se usa aqui
    tcp->loop = loop_s;

    loopFSM_t* loop = loop_s->loopFSM->user_data;

    tcp->alloc_cb = NULL;
    tcp->close_cb = NULL;
    tcp->connection_cb = NULL;
    tcp->read_cb = NULL;

    // add handler to handler list in loop
    uv_stream_t** handlers = loop->active_stream_handlers;
    int i = loop->n_active_stream_handlers; // array index

    if(loop->n_active_stream_handlers == 0){
    *handlers = malloc(sizeof(uv_stream_t));
    memcpy((uv_stream_t*)handlers[0], (uv_stream_t*) tcp, sizeof(uv_stream_t));
    } else {
    *handlers = realloc(*handlers, sizeof(uv_stream_t[i]));
    memcpy((uv_stream_t*)handlers[i], (uv_stream_t*) tcp, sizeof(uv_stream_t));
    }

    loop->n_active_stream_handlers++;
}

int
uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags){
    unsigned int addrlen;

    // Can only be used in ipv4 version
    // this function creates a socket (AF_INET4) and stores it inside handle as a fd
    // sets sockets options (SOL_SOCKET, SO_REUSEADDR, etc)
    // finally binds the socket to the IPv4 address

    // PROBLEMS -> sockaddr exists in esp8266 rtos?
}

int
uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct  sockaddr* addr, uv_connect_cb cb){
    // this function connects the socket in handle to addr in sockeaddr
    // once connection is completed callback is triggered (added to the correspondant state in loop FSM to be executed once)
    // this connection cb is a handshake or similar. That why a pollout whatcher is needed to send handshake msg
}

int
uv_tcp_close(uv_loop_t* loop, uv_tcp_t* tcp){
    // close socket, pull out watcher
}