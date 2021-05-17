#include "uv.h"

#define GPIO 16

void signal_cb(uv_signal_t* signal, int signum);
void moreInfo_cb(uv_timer_t* timer);
void connection_cb(uv_stream_t* server, int status);
void w_cb(uv_write_t* req, int status);
void server_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void server_read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf);

void
main_everything_server(void* ignore){
    /* Configure GPIO */
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL<<GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    /* Init loop */
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));

    uv_loop_init(loop);

    /* Init tcp server */
    uv_tcp_t* tcp = malloc(sizeof(uv_tcp_t));

    uv_tcp_init(loop, tcp);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
    
    uv_tcp_bind(tcp, (struct sockaddr*)&server_addr, 0);

    uv_listen((uv_stream_t*)tcp, 1, connection_cb);

    /* Run loop */
    uv_run(loop, UV_RUN_DEFAULT);

    vTaskDelete(NULL);
}

void
signal_cb(uv_signal_t* signal, int signum){
    uv_stream_t* client = signal->data;
    uv_buf_t* buf = malloc(sizeof(uv_buf_t));
    uv_write_t* req = malloc(sizeof(uv_write_t));

    buf->len = 4*1024;
    buf->base = malloc(4*1024);
    strcpy(buf->base, "TEMPHUM");

    uv_write(req, client, buf, 1, w_cb);

    uv_read_start(client, server_alloc_cb, server_read_cb);
}

void
moreInfo_cb(uv_timer_t* timer){
    uv_stream_t* client = timer->data;
    uv_buf_t* buf = malloc(sizeof(uv_buf_t));
    uv_write_t* req = malloc(sizeof(uv_write_t));

    buf->len = 4*1024;
    buf->base = malloc(4*1024);
    strcpy(buf->base, "TEMPHUM\0");

    uv_write(req, client, buf, 1, w_cb);
}

void
connection_cb(uv_stream_t* server, int status){
    ESP_LOGI("CONNECTION_CB", "Connection callback has been called with status = %d", status);
    // If listen has failed, try again
    if(status != 0){

        uv_listen(server, 1, connection_cb);
        return;
    }

    uv_stream_t* client = malloc(sizeof(uv_stream_t));

    uv_accept(server, client);

    /* Init signal handles */
    uv_signal_t* signal = malloc(sizeof(uv_signal_t));

    uv_signal_init(server->loop, signal);
    signal->data = client;
    uv_signal_start(signal, signal_cb, GPIO);

    /* Init timer handle */
    uv_timer_t* timer = malloc(sizeof(uv_timer_t));

    uv_timer_init(server->loop, timer);
    timer->data = client;
    uv_timer_start(timer, moreInfo_cb, 60000, 60000);

}

void
w_cb(uv_write_t* req, int status){
    free(req->bufs[0].base);
    free(req->bufs);
}

void 
server_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    buf->len = suggested_size;
    buf->base = malloc(suggested_size);
}

void
server_read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf){
    ESP_LOGI("server_read_cb", "%d %s", nread, buf->base);
    if(nread < 0)
        return;

    if(nread == buf->len){
        ESP_LOGI("server_read_cb", "Read %d , %s", nread, buf->base);
    }

    free(buf->base);
    free(buf);

    uv_read_stop(stream);
    uv_read_start(stream, server_alloc_cb, server_read_cb);
}