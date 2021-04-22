#include "uv.h"
#include "init.h"

/* INDICAR EJEMPLO 1 o 2 (para configurar la dirección IP)*/
#define EJEMPLO 2


/*  Ejemplo 1
 *  Cambiando la constante de preprocesador SERVER_CLIENT a 0 o 1 indica el comportamiento que se cargará
 *  Si SERVER_CLIENT = 0 -> Se pretende crear un server en 192.168.0.200:50000 que acepte una conexión y mande continuamente un mensaje
 *  Si SERVER_CLIENT = 1 -> Se pretende crear un cliente en 192.168.0.201:50001 que se conecte al server e intente recibir continuamente un mensaje
 *  IMPORTANTE -> en init.h cambiar la ssid y password de la red wifi
 *  Los diferentes pasos se comentan a lo largo del codigo
 * 
 *  Comportamiento esperado -> servidor enviando mensajes continuamente, cliente recibiéndolos continuamente
 *  Comportamiento observado -> servidor envía correctamente la primera vez, entonces se queda bloqueado
 *                              cliente recibe correctamente la primera vez, entonces se queda bloqueado
 *                              en ocasiones no se consigue realizar el emparejamiento y el cliente muestra errores al hacer connect
*/

#define SERVER_CLIENT 1

#if(SERVER_CLIENT == 0)
    #define LOCALIP "192.168.0.200"
    #define CLIENTIP "192.168.0.201"
#else
    #define CLIENTIP "192.168.0.200"
    #define LOCALIP "192.168.0.201"
#endif

uv_tcp_t* tcp_client;

/* Función llamada desde libuv para allocatar espacio (se llama dentro de uv_read_start y solo una vez) */
void
alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    buf->len = suggested_size;
    buf->base = malloc(suggested_size);
}

/* Función llamada cada vez que se lee algo (se llama desde uv_read_start cada vez que se lee algo) */
void
read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf){
    if(nread < 0)
        return;

    if(nread == buf->len){ // stop reading, everything expected has been read
        ESP_LOGI("READ_CB", "Everything received = %s", buf->base);
        free(buf->base);
        free(buf);
        uv_read_stop(stream);
        uv_read_start(stream, alloc_cb, read_cb);
        return;
    } else if(nread == 0){ // more info is going to be read, well be called afterwards
        return;
    } else if(nread < buf->len){ // more info is going to be read, well be called afterwards
        buf->base += nread;
        buf->len -= nread;
        return;
    }
}

/* Función llamada cuando se realiza una conexión con un servidor desde el lado cliente (uv_tcp_connect) */
void
connect_cb(uv_connect_t* req, int status){
    int rv;
    ESP_LOGI("connect_cb", "Entered connect_cb with status = %d", status);

    rv = uv_read_start((uv_stream_t*)tcp_client, alloc_cb, read_cb);
    if(rv != 0){
        ESP_LOGE("connect_cb","Error while trying to read_start");
    }
}

/* Función llamada cuando se completa una escritura desde el lado servidor (uv_write)*/
void
write_cb(uv_write_t* req, int status){
    int rv;

    ESP_LOGI("WRITE_CB", "Write callback has been called with status = %d", status);

    free(req->bufs);
    uv_buf_t* buf = malloc(sizeof(uv_buf_t));
    memset(buf, 0, sizeof(uv_buf_t));
    buf->base = malloc(4*1024);
    strcpy(buf->base, "Saludos");
    buf->len = 4*1024;

    ESP_LOGI("write_cb", "uv_write");
    rv = uv_write(req, req->stream, buf, 1, write_cb);
    if(rv != 0){
        ESP_LOGE("write_cb","Error while trying to write");
    }
}

/* Función llamada desde el lado servidor como cb de uv_listen */
void
connection_cb(uv_stream_t* server, int status){
    int rv;
    ESP_LOGI("CONNECTION_CB", "Connection callback has been called with status = %d", status);

    uv_stream_t* client = malloc(sizeof(uv_stream_t));
    memset(client, 0, sizeof(uv_stream_t));
    ESP_LOGI("CONNECTION_CB", "uv_accept");
    rv = uv_accept(server, client);
    if(rv != 0){
        ESP_LOGE("CONNECTION_CB", "Error while trying to accept");
    }

    uv_write_t* req = malloc(sizeof(uv_write_t));
    uv_buf_t* buf = malloc(sizeof(uv_buf_t));
    memset(buf, 0, sizeof(uv_buf_t));
    buf->base = malloc(4*1024);
    strcpy(buf->base, "Saludos");
    buf->len = 4*1024;

    ESP_LOGI("CONNECTION_CB", "uv_write");
    rv = uv_write(req, client, buf, 1, write_cb);
    if(rv != 0){
        ESP_LOGE("TCP_TIMER_CB","Error while trying to write");
    }
}

void
example1(void* ignore){ 

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_tcp");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_tcp");

    /* Init server side */
    if(SERVER_CLIENT == 0){
        uv_tcp_t* tcp_server = malloc(sizeof(uv_tcp_t));

        rv = uv_tcp_init(loop, tcp_server);
        if(rv != 0){
            ESP_LOGE("TCP_INIT", "Error in first uv_tcp_init in main_tcp");
        }

        ESP_LOGI("TCP_INIT", "First uv_tcp_init success");

        struct sockaddr_in local_addr;
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(50000);
        local_addr.sin_addr.s_addr = inet_addr(LOCALIP);

        rv = uv_tcp_bind(tcp_server, (struct sockaddr*)&local_addr, 0);
        if(rv != 0){
            ESP_LOGE("TCP_BIND", "Error in uv_tcp_bind in main_tcp");
        }

        ESP_LOGI("TCP_BIND", "uv_tcp_bind success");

        rv = uv_listen((uv_stream_t*)tcp_server, 1, connection_cb);
        if(rv != 0){
            ESP_LOGE("UV_LISTEN", "Error in uv_listen in main_tcp");
        }

        ESP_LOGI("UV_LISTEN", "uv_listen success");
    } else{

        /* Init client side */
        tcp_client = malloc(sizeof(uv_tcp_t));

        rv = uv_tcp_init(loop, tcp_client);
        if(rv != 0){
            ESP_LOGE("TCP_INIT", "Error in first uv_tcp_init in main_tcp");
        }

        ESP_LOGI("TCP_INIT", "First uv_tcp_init success");

        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(50001);
        client_addr.sin_addr.s_addr = inet_addr(LOCALIP);

        rv = uv_tcp_bind(tcp_client, (struct sockaddr*)&client_addr, 0);
        if(rv != 0){
            ESP_LOGE("TCP_BIND", "Error in uv_tcp_bind in main_tcp");
        }

        ESP_LOGI("TCP_BIND", "uv_tcp_bind success");

        uv_connect_t* req = malloc(sizeof(uv_connect_t));

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(50000);
        server_addr.sin_addr.s_addr = inet_addr(CLIENTIP);

        rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
        if(rv != 0){
            ESP_LOGE("TCP_CONNECT", "Error in uv_tcp_connect in main_tcp");
    }

    ESP_LOGI("TCP_CONNECT", "uv_tcp_connect success");
    } 

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_tcp");
    }

    vTaskDelete(NULL);
}

/*  Ejemplo 2
 *  Cambiando la constante de preprocesador DIR a 0 o 1 indica la dirección IP
 *  Si DIR = 0 -> Se pretende crear un server/cliente en 192.168.0.200:50000
 *  Si DIR = 1 -> Se pretende crear un server/cliente en 192.168.0.201:50000
 *  Ambos nodos deben mandar y recibir mensajes continuamente
 *  IMPORTANTE -> en init.h cambiar la ssid y password de la red wifi
 *  Los diferentes pasos se comentan a lo largo del codigo
 * 
 *  Comportamiento esperado -> intercambio de mensajes constante
 *  Comportamiento observado -> no se consigue ningun emparejamiento,
 *  ambos client dan un errno 104 (connection reet by peer), después errno 128 (key has been revoked)              
*/

#define DIR 0

#if(DIR == 0)
    #define IP "192.168.0.200"
    #define SECOND_IP "192.168.0.201"
#else
    #define IP "192.168.0.201"
    #define SECOND_IP "192.168.0.200"
#endif


void
example2(void* ignore){ 

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_tcp");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_tcp");

    /* Init server side */
    uv_tcp_t* tcp_server = malloc(sizeof(uv_tcp_t));

    rv = uv_tcp_init(loop, tcp_server);
    if(rv != 0){
        ESP_LOGE("TCP_INIT", "Error in first uv_tcp_init in main_tcp");
    }

    ESP_LOGI("TCP_INIT", "First uv_tcp_init success");

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(50000);
    local_addr.sin_addr.s_addr = inet_addr(IP);

    rv = uv_tcp_bind(tcp_server, (struct sockaddr*)&local_addr, 0);
    if(rv != 0){
        ESP_LOGE("TCP_BIND", "Error in uv_tcp_bind in main_tcp");
    }

    ESP_LOGI("TCP_BIND", "uv_tcp_bind success");

    rv = uv_listen((uv_stream_t*)tcp_server, 1, connection_cb);
    if(rv != 0){
        ESP_LOGE("UV_LISTEN", "Error in uv_listen in main_tcp");
    }

    ESP_LOGI("UV_LISTEN", "uv_listen success");

    /* Init client side */
    tcp_client = malloc(sizeof(uv_tcp_t));

    rv = uv_tcp_init(loop, tcp_client);
    if(rv != 0){
        ESP_LOGE("TCP_INIT", "Error in first uv_tcp_init in main_tcp");
    }

    ESP_LOGI("TCP_INIT", "First uv_tcp_init success");

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(50001);
    client_addr.sin_addr.s_addr = inet_addr(IP);

    rv = uv_tcp_bind(tcp_client, (struct sockaddr*)&client_addr, 0);
    if(rv != 0){
        ESP_LOGE("TCP_BIND", "Error in uv_tcp_bind in main_tcp");
    }

    ESP_LOGI("TCP_BIND", "uv_tcp_bind success");

    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000);
    server_addr.sin_addr.s_addr = inet_addr(SECOND_IP);

    rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
    if(rv != 0){
        ESP_LOGE("TCP_CONNECT", "Error in uv_tcp_connect in main_tcp");
    }

    ESP_LOGI("TCP_CONNECT", "uv_tcp_connect success");

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_tcp");
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI("APP_MAIN", "Beggining app_main");
    vTaskDelay(5000/portTICK_RATE_MS);

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init WiFi with static IP
    if(EJEMPLO == 1){
        wifi_init(LOCALIP);
        xTaskCreate(example1, "startup", 16384, NULL, 5, NULL);
    } else {
        wifi_init(IP);
        xTaskCreate(example2, "startup", 16384, NULL, 5, NULL);
    }
}