#include "uv.h"

int
uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb){
    // This function should only safe information given through the arguments
    ((uv_tcp_t*)stream)->connection_cb = cb;
}

int
uv_accept(uv_stream_t* server, uv_stream_t* client){
    // This function should call espconn_accept(struct espconn* espconn) after building the corresponding argument
    // espconn_recv_hold?
}

int
uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb){
    // espconn_recv_unhold
    ((uv_tcp_t*)stream)->alloc_cb = alloc_cb;
    ((uv_tcp_t*)stream)->read_cb = read_cb;
}

int
uv_read_stop(uv_stream_t* stream){
    // espconn_recv_hold
}

int
uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb){
    ((uv_tcp_t*)handle)->write_cb = cb;
    // espconn_send
}