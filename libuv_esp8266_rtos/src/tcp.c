#include "uv.h"

int
uv_tcp_init(uv_loop_t* loop, uv_tcp_t* tcp){
    uv__stream_init(loop, (uv_stream_t*)tcp, UV_TCP); // this should initialize tcp's watcher
    tcp->loop = loop;
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
    // adds a watcher on the connection (add to the loop in watcher state)
}

int
uv_tcp_close(uv_loop_t* loop, uv_tcp_t* tcp){
    // close socket, pull out watcher
}