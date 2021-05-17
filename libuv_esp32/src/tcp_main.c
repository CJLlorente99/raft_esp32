// #include "uv.h"

// #define N_SERVERS 3

// typedef struct{
//     uv_loop_t* loop;
//     char ip[64];
//     int port;
//     uv_tcp_t* tcp;
// } data_t;

// typedef struct{
//     uv_stream_t* client;
//     uv_stream_t* server;
//     int port;
// } data1_t;

// /* Client callbacks */

// void
// alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
//     ESP_LOGI("alloc_cb", "entering");
//     buf->len = suggested_size;
//     buf->base = malloc(suggested_size);
//     ESP_LOGI("alloc_cb", "exiting");
// }

// void
// read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf){
//     ESP_LOGI("read_cb", "entering");
//     if(nread < 0)
//         return;

//     if(nread == buf->len){ // stop reading, everything expected has been read
//         ESP_LOGI("READ_CB", "Everything received = %s", buf->base+nread-4096);
//         // free(buf->base);
//         // free(buf);
//         uv_read_stop(stream);
//         return;
//     } else if(nread == 0){ // more info is going to be read, well be called afterwards
//         return;
//     } else if(nread < buf->len){ // more info is going to be read, well be called afterwards
//         buf->len -= nread;
//         buf->base += nread;
//         return;
//     }
//     ESP_LOGI("read_cb", "exiting");
// }

// void
// timer_client(uv_timer_t* handle){
//     ESP_LOGI("timer_client", "entering");
//     uv_tcp_t* tcp_client = handle->data;
//     uv_read_start((uv_stream_t*)tcp_client, alloc_cb, read_cb);
//     ESP_LOGI("timer_client", "exiting");
// }

// void connect_cb(uv_connect_t* req, int status);

// void
// resetSocketCb(uv_handle_t* handle){
//     ESP_LOGI("resetSocketCb", "entering");
//     data_t* data = handle->data;
//     char* ip = data->ip;
//     uv_loop_t* loop = data->loop;
//     uv_tcp_t* tcp_client = data->tcp;
//     tcp_client = malloc(sizeof(uv_tcp_t));
//     int rv;

//     rv = uv_tcp_init(loop, tcp_client);
//     if(rv != 0){
//         ESP_LOGE("resetSocketCb", "uv_tcp_init");
//     }

//     uv_connect_t* req = malloc(sizeof(uv_connect_t));

//     struct sockaddr_in server_addr;
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(50000);
//     server_addr.sin_addr.s_addr = inet_addr(ip);

//     req->data = data;

//     rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
//     if(rv != 0){
//         ESP_LOGE("resetSocketCb", "uv_tcp_connect");
//     }
//     ESP_LOGI("resetSocketCb", "exiting");
// }

// void
// connect_cb(uv_connect_t* req, int status){
//     ESP_LOGI("connect_cb", "entering");
//     ESP_LOGI("connect_cb", "Entered connect_cb with status = %d", status);
//     data_t* data = req->data;
    
//     // Try to connect again if connection has failed
//     if(status != 0){
//         if(errno == 104){
//             data->tcp->data = req->data;
//             uv_close((uv_handle_t*)data->tcp, resetSocketCb);
//         }
//         ESP_LOGI("connect_cb", "exiting");
//         return;
//     }
//     free(data);

//     uv_timer_t* timer = malloc(sizeof(uv_timer_t));
//     uv_timer_init(req->loop, timer);
//     timer->data = req->tcp;
//     uv_timer_start(timer, timer_client, 0, 2000);
//     ESP_LOGI("connect_cb", "exiting");
// }

// /* Server callbacks */
// void timer_cb(uv_timer_t* handle);
// void connection_cb(uv_stream_t* server, int status);

// void
// resetServerCb(uv_handle_t* handle){
//     ESP_LOGI("resetServerCb", "entering");
//     data1_t* data = handle->data;
//     uv_tcp_t* server = (uv_tcp_t*)data->server;
//     int rv;

//     rv = uv_tcp_init(handle->loop, server);
//     if(rv != 0){
//         ESP_LOGE("resetServerCb", "uv_tcp_init");
//     }

//     free(data->client);

//     struct sockaddr_in local_addr_1;
//     local_addr_1.sin_family = AF_INET;
//     local_addr_1.sin_port = htons(data->port);
//     local_addr_1.sin_addr.s_addr = inet_addr(SERVERIP);

//     int* port = malloc(sizeof(int));
//     *port = 50000;
//     server->data = port;

//     rv = uv_tcp_bind(server, (struct sockaddr*)&local_addr_1, 0);
//     if(rv != 0){
//         ESP_LOGE("resetServerCb", "uv_tcp_bind");
//     }

//     rv = uv_listen((uv_stream_t*)server, 2, connection_cb);
//     if(rv != 0){
//         ESP_LOGE("resetServerCb", "uv_listen");
//     }
//     ESP_LOGI("resetServerCb", "exiting");
// }

// void
// write_cb(uv_write_t* req, int status){
//     ESP_LOGI("write_cb", "entering");
//     data1_t* data = req->data;

//     free(req->bufs->base);
//     free(req->bufs);
//     ESP_LOGI("WRITE_CB", "Write callback has been called with status = %d", status);
//     // Try to connect again if connection has failed
//     if(status != 0){
//         if(errno == 104){
//             data->server->data = req->data;
//             uv_close((uv_handle_t*)data->server, resetServerCb);
//         }
//         ESP_LOGI("write_cb", "exiting");
//         return;
//     }

//     uv_timer_t* timer = malloc(sizeof(uv_timer_t));
//     uv_timer_init(req->loop, timer);
//     timer->data = req->stream;
//     uv_timer_start(timer, timer_cb, 2000, 0);

//     ESP_LOGI("write_cb", "exiting");
// }

// void
// timer_cb(uv_timer_t* handle){
//     ESP_LOGI("timer_cb", "entering");
//     int rv;
//     data1_t* data_server = handle->data;

//     uv_write_t* req = malloc(sizeof(uv_write_t));
//     uv_buf_t* buf = malloc(sizeof(uv_buf_t));
//     memset(buf, 0, sizeof(uv_buf_t));
//     buf->base = malloc(4*1024);
//     sprintf(buf->base, "Saludos Server %d", ID);
//     buf->len = 4*1024;

//     data_t* data = malloc(sizeof(data_t));
//     strcpy(data->ip, SERVERIP);
//     data->loop = handle->loop;
//     data->port = data_server->port;
//     data->tcp = (uv_tcp_t*)data_server->server;
//     req->data = data;

//     rv = uv_write(req, data_server->client, buf, 1, write_cb);
//     if(rv != 0){
//         ESP_LOGE("timer_cb","uv_write");
//     }

//     uv_timer_stop(handle);
//     uv_close((uv_handle_t*)handle, NULL);

//     ESP_LOGI("timer_cb", "exiting");
// }

// void
// connection_cb(uv_stream_t* server, int status){
//     ESP_LOGI("connection_cb", "entering");
//     int rv;
//     ESP_LOGI("CONNECTION_CB", "Connection callback has been called with status = %d", status);
//     int* port = (int*)server->data;
//     // If listen has failed, try again
//     if(status != 0){

//         rv = uv_listen(server, 1, connection_cb);
//         if(rv != 0){
//             ESP_LOGE("connection_cb", "uv_listen");
//         }
//         ESP_LOGI("timer_cb", "exiting");
//         return;
//     }

//     uv_stream_t* client = malloc(sizeof(uv_stream_t));

//     rv = uv_accept(server, client);
//     if(rv != 0){
//         ESP_LOGE("connection_cb", "uv_accept");
//     }

//     data1_t* data = malloc(sizeof(data1_t));
//     data->port = *port;
//     data->client = client;
//     data->server = server;
//     uv_timer_t* timer_1 = malloc(sizeof(uv_timer_t));
//     rv = uv_timer_init(server->loop, timer_1);
//     timer_1->data = data;
//     rv = uv_timer_start(timer_1, timer_cb, 2000, 0);

//     free(port);

//     ESP_LOGI("connection_cb", "exiting");
// }

// void
// startClientCb(uv_timer_t* timer){
//     ESP_LOGI("startClientCb", "entering");
//     int rv;
//     data_t* data = timer->data;
//     uv_loop_t* loop = data->loop;
//     char* ip = data->ip;
//     int port = data->port;
//     uv_tcp_t* tcp_client = data->tcp;

//     rv = uv_tcp_init(loop, tcp_client);
//     if(rv != 0){
//         ESP_LOGE("startClientCb", "uv_tcp_init");
//     }

//     uv_connect_t* req = malloc(sizeof(uv_connect_t));

//     struct sockaddr_in server_addr;
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(port);
//     server_addr.sin_addr.s_addr = inet_addr(ip);

//     req->data = data;

//     rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
//     if(rv != 0){
//         ESP_LOGE("startClientCb", "uv_tcp_connect");
//     }

//     uv_timer_stop(timer);
//     ESP_LOGI("startClientCb", "exiting");
// }

// /* main */
// void
// main_tcp(void* ignore){ 

//     /* Init loop */
//     uv_loop_t* loop = malloc(sizeof(uv_loop_t));
//     int rv;

//     rv = uv_loop_init(loop);
//     if(rv != 0){
//         ESP_LOGE("main_tcp","uv_loop_init");
//     }

//     /* Init server side */
//     uv_tcp_t* tcp_server_1 = malloc(sizeof(uv_tcp_t));

//     rv = uv_tcp_init(loop, tcp_server_1);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_tcp_init");
//     }

//     struct sockaddr_in local_addr_1;
//     local_addr_1.sin_family = AF_INET;
//     local_addr_1.sin_port = htons(50000);
//     local_addr_1.sin_addr.s_addr = inet_addr(SERVERIP);

//     int* port1 = malloc(sizeof(int));
//     *port1 = 50000;
//     tcp_server_1->data = port1;

//     rv = uv_tcp_bind(tcp_server_1, (struct sockaddr*)&local_addr_1, 0);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_tcp_bind");
//     }

//     rv = uv_listen((uv_stream_t*)tcp_server_1, 2, connection_cb);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_listen");
//     }

//     /* 2 */
//     uv_tcp_t* tcp_server_2 = malloc(sizeof(uv_tcp_t));

//     rv = uv_tcp_init(loop, tcp_server_2);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_tcp_init");
//     }

//     struct sockaddr_in local_addr_2;
//     local_addr_2.sin_family = AF_INET;
//     local_addr_2.sin_port = htons(49999);
//     local_addr_2.sin_addr.s_addr = inet_addr(SERVERIP);

//     int* port2 = malloc(sizeof(int));
//     *port2 = 49999;
//     tcp_server_2->data = port2;

//     rv = uv_tcp_bind(tcp_server_2, (struct sockaddr*)&local_addr_2, 0);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_tcp_bind");
//     }

//     rv = uv_listen((uv_stream_t*)tcp_server_2, 2, connection_cb);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_listen");
//     }

//     /* Init client side */
//     uv_timer_t* timer_client[3];
//     data_t* data[3];
//     int port = 49999;

//     for(int i = 0; i < N_SERVERS; i++){
//         if(i+1 == ID)
//             continue;
//         timer_client[i] = malloc(sizeof(uv_timer_t));
//         data[i] = malloc(sizeof(data_t));
//         uv_timer_init(loop, timer_client[i]);
//         char ip[64];
//         sprintf(ip, "192.168.0.20%d", i+1);
//         strcpy(data[i]->ip, ip);
//         data[i]->loop = loop;
//         data[i]->tcp = malloc(sizeof(uv_tcp_t));
//         data[i]->port = port++;
//         timer_client[i]->data = data[i];
//         uv_timer_start(timer_client[i], startClientCb, 1000, 0);
//     }    

//     rv = uv_run(loop, UV_RUN_DEFAULT);
//     if(rv != 0){
//         ESP_LOGE("main_tcp", "uv_run");
//     }

//     vTaskDelete(NULL);
// }