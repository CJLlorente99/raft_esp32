#include "uv.h"
#include "../ESP32-DHT11/include/dht11.h"

#define DHT11GPIO 16
#define DATAFILE "data"

#define SERVERIP "192.168.0.200"
#define SERVERPORT 50000

void connect_cb(uv_connect_t* req, int status);
void resetSocketCb(uv_handle_t* handle);
void timer_cb(uv_timer_t* timer);
void read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf);
void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void write_cb(uv_write_t* req, int status);

void
main_everything_client(void* ignore){
    /* Init dht11 */
    DHT11_init(DHT11GPIO);

    /* Init loop */
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));

    uv_loop_init(loop);

    /* Init tcp client */

    uv_tcp_t* client = malloc(sizeof(uv_tcp_t));
    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);

    req->data = client;

    uv_tcp_init(loop, client);
    uv_tcp_connect(req, client, (struct sockaddr*)&server_addr, connect_cb);

    /* Init timer for collecting data periodically */

    uv_timer_t* timer = malloc(sizeof(uv_timer_t));

    uv_timer_init(loop, timer);
    uv_timer_start(timer, timer_cb, 0, 1000);

    /* Run loop */

    uv_run(loop, UV_RUN_DEFAULT);

    vTaskDelete(NULL);
}

void
alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    buf->len = suggested_size;
    buf->base = malloc(suggested_size);
}

void
read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf){
    if(nread < 0){
        return;
    }

    if(nread == buf->len){
        ESP_LOGI("read_cb", "Received: %s", buf->base);

        char str[4096];
        strcpy(str, "TEMPHUM\0");

        if(strncmp(buf->base, str, sizeof("TEMPHUM"))){
            uv_write_t* req = malloc(sizeof(uv_write_t));
            uv_buf_t* buf = malloc(sizeof(uv_buf_t));
            buf->base = malloc(4*1024);
            buf->len = 0;
            uv_fs_t fs_req;
            uv_file file;
            int n;

            uv_fs_stat(NULL, &fs_req, DATAFILE, NULL);
            file = uv_fs_open(NULL, &fs_req, DATAFILE, UV_FS_O_RDONLY, 0, NULL);
            buf->len = fs_req.statbuf.st_size;
            n = uv_fs_read(NULL, &fs_req, file, buf, 1, 0, NULL);
            if(n != fs_req.statbuf.st_size){
                ESP_LOGE("read_cb", "ERROR. Expected to read %d, read %d", buf->len, n);
                while(1){}
            }
            uv_fs_close(NULL, &fs_req, file, NULL);
            uv_fs_unlink(NULL,&fs_req, DATAFILE, NULL);

            uv_write(req, stream, buf, 1, write_cb);
        }

        free(buf->base);
        free(buf);
        uv_read_stop(stream);
        uv_read_start(stream, alloc_cb, read_cb);
    }
}

void
resetSocketCb(uv_handle_t* handle){
    uv_tcp_t* client = malloc(sizeof(uv_tcp_t));

    uv_tcp_init(handle->loop, client);

    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);

    req->data = client;

    uv_tcp_connect(req, client, (struct sockaddr*)&server_addr, connect_cb);
}

void
connect_cb(uv_connect_t* req, int status){
    ESP_LOGI("connect_cb", "Entered connect_cb with status = %d", status);
    
    // Try to connect again if connection has failed
    if(status != 0){
        uv_close((uv_handle_t*)req->data, resetSocketCb);
        ESP_LOGI("connect_cb", "exiting");
        return;
    }

    uv_read_start((uv_stream_t*)req->data, alloc_cb, read_cb);
}

void
timer_cb (uv_timer_t* timer){
    struct dht11_reading reading = DHT11_read();
    uv_fs_t req;
    uv_buf_t buf;
    uv_file file;
    char str[100];

    sprintf(str, "TEMP:%d,HUM:%d\n", reading.temperature, reading.humidity);

    buf.base = malloc(100);
    strcpy(buf.base, str);
    buf.len = strlen(buf.base);

    file = uv_fs_open(NULL, &req, DATAFILE, UV_FS_O_WRONLY|UV_FS_O_APPEND, 0, NULL);
    uv_fs_write(NULL, &req, &file, &buf, 1, 0, NULL);
    uv_fs_fsync(NULL, &req, file, NULL);
    uv_fs_close(NULL, &req, file, NULL);

    free(buf.base);
}

void
write_cb(uv_write_t* req, int status){
    ESP_LOGI("write_cb", "written");
    free(req->bufs[0].base);
    free(req->bufs);
}