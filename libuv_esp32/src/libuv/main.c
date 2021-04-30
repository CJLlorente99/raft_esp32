/* This file should contain some code that verifies libuv implementation correctness
To be verified
    signals -> some buttons and led (when one is pressed all leds on, when the other is pressed all leds off)

*/

#include "uv.h"
#include "init.h"

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle;

// SIGNAL TEST

#define INPUT_TEST_PORT_OFF 22
#define INPUT_TEST_PORT_ON 19
#define LED_TEST_PORT 14
// #define LED_DEBUG_PORT 5

void
test_callback_on (uv_signal_t* handle, int signum){
    gpio_set_level(LED_TEST_PORT,1);
}

void
test_callback_off (uv_signal_t* handle, int signum){
    gpio_set_level(LED_TEST_PORT,0);
}

void
main_signal(void* ignore){
    // Configure GPIO
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = 1ULL<<LED_TEST_PORT;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL<<INPUT_TEST_PORT_OFF)|(1ULL<<INPUT_TEST_PORT_ON));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_signal");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_signal");

    // Init signal handle
    uv_signal_t* signal_handle_on = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_handle_off = malloc(sizeof(uv_signal_t));

    rv = uv_signal_init(loop, signal_handle_on);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la primera inicializacion en main_signal");
    }

    ESP_LOGI("SIGNAL_INIT", "Primer signal inicializado en main_signal");

    // Aqui hay un error
    rv = uv_signal_start(signal_handle_on, test_callback_on, INPUT_TEST_PORT_ON);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante primer uv_signal_start en main_signal");
    }

    ESP_LOGI("SIGNAL_START", "Primer uv_signal_start en main_signal");

    rv = uv_signal_init(loop, signal_handle_off);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la segunda inicializacion en main_signal");
    }

    ESP_LOGI("SIGNAL_INIT", "Segundo signal inicializado en main_signal");

    rv = uv_signal_start(signal_handle_off, test_callback_off, INPUT_TEST_PORT_OFF);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante segundo uv_signal_start en main_signal");
    }

    ESP_LOGI("SIGNAL_START", "Segundo uv_signal_start en main_signal");

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("UV_RUN","Error durante uv_run");
    }

    vTaskDelete(NULL);
}

// TIMER TEST

// #define ONE_SHOT_CLOCK_TRIGGER_PORT 15
// #define LED_ONE_SHOT_PORT 17
// #define LED_TWO_SEC_PORT 18
#define LED_HALF_SEC_PORT 14

void
half_sec_callback_on (uv_timer_t* handle){
    gpio_set_level(LED_HALF_SEC_PORT,1);
    ESP_LOGI("LED", "LED ENCENDIDA");
}

void
half_sec_callback_off (uv_timer_t* handle){
    gpio_set_level(LED_HALF_SEC_PORT,0);
    ESP_LOGI("LED", "LED APAGADA");
}

void
main_timer(void* ignore){ 

    // Configure GPIO
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<LED_HALF_SEC_PORT);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_signal");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_signal");

    // Init timers
    uv_timer_t* timer_half_sec_on = malloc(sizeof(uv_timer_t));
    uv_timer_t* timer_half_sec_off = malloc(sizeof(uv_timer_t));

    rv = uv_timer_init(loop, timer_half_sec_on);
    if(rv != 0){
        ESP_LOGE("TIMER_INIT", "Error in first uv_timer_init in main_timer");
    }

    ESP_LOGI("TIMER_INIT", "First uv_timer_init success");

    rv = uv_timer_init(loop, timer_half_sec_off);
    if(rv != 0){
        ESP_LOGE("TIMER_INIT", "Error in second uv_timer_init in main_timer");
    }

    ESP_LOGI("TIMER_INIT", "Second uv_timer_init success");

    rv = uv_timer_start(timer_half_sec_on, half_sec_callback_on, 0, 2000);
    if(rv != 0){
        ESP_LOGE("TIMER_START", "Error in first uv_timer_start in main_timer");
    }

    ESP_LOGI("TIMER_START", "First uv_timer_start success");

    rv = uv_timer_start(timer_half_sec_off, half_sec_callback_off, 1000, 2000);
    if(rv != 0){
        ESP_LOGE("TIMER_START", "Error in second uv_timer_start in main_timer");
    }

    ESP_LOGI("TIMER_START", "Second uv_timer_start success");

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_timer");
    }

    vTaskDelete(NULL);
}

// TCP test server

#define LOCALIP "192.168.0.200"
#define CLIENTIP "192.168.0.201"

// #define CLIENTIP "192.168.0.200"
// #define LOCALIP "192.168.0.201"

uv_tcp_t* tcp_server;
uv_tcp_t* tcp_client;

void
alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
    buf->len = suggested_size;
    buf->base = malloc(suggested_size);
}

void
read_cb(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf){
    if(nread < 0)
        return;

    if(nread == buf->len){ // stop reading, everything expected has been read
        ESP_LOGI("READ_CB", "Everything received = %s", buf->base+nread-4096);
        // free(buf->base);
        // free(buf);
        uv_read_stop(stream);
        return;
    } else if(nread == 0){ // more info is going to be read, well be called afterwards
        return;
    } else if(nread < buf->len){ // more info is going to be read, well be called afterwards
        buf->len -= nread;
        buf->base += nread;
        return;
    }
}

void
timer_client(uv_timer_t* handle){
    uv_read_start((uv_stream_t*)tcp_client, alloc_cb, read_cb);
}
void connect_cb(uv_connect_t* req, int status);
void
resetSocketCb(uv_handle_t* handle){
    uv_loop_t* loop = handle->data;
    int rv;

    rv = uv_tcp_init(loop, tcp_client);
    if(rv != 0){
        ESP_LOGE("resetSocketCb", "Error in first uv_tcp_init in main_tcp");
    }

    ESP_LOGI("resetSocketCb", "First uv_tcp_init success");

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(50001);
    client_addr.sin_addr.s_addr = inet_addr(LOCALIP);

    rv = uv_tcp_bind(tcp_client, (struct sockaddr*)&client_addr, 0);
    if(rv != 0){
        ESP_LOGE("resetSocketCb", "Error in uv_tcp_bind in main_tcp");
    }

    ESP_LOGI("resetSocketCb", "uv_tcp_bind success");

    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000);
    server_addr.sin_addr.s_addr = inet_addr(CLIENTIP);

    rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
    if(rv != 0){
        ESP_LOGE("resetSocketCb", "Error in uv_tcp_connect in main_tcp");
    }

    ESP_LOGI("resetSocketCb", "uv_tcp_connect success"); 
}

void
connect_cb(uv_connect_t* req, int status){
    ESP_LOGI("connect_cb", "Entered connect_cb with status = %d", status);

    // Try to connect again if connection has failed
    if(status != 0){
        if(errno == 104){
            tcp_client->data = req->loop;
            uv_close(tcp_client, resetSocketCb);
        }
        return;
    }

    uv_timer_t* timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(req->loop, timer);
    uv_timer_start(timer, timer_client, 0, 2000);
}

void timer_cb(uv_timer_t* handle);
void
write_cb(uv_write_t* req, int status){
    int rv;

    free(req->bufs->base);
    free(req->bufs);
    ESP_LOGI("WRITE_CB", "Write callback has been called with status = %d", status);

    uv_timer_t* timer = malloc(sizeof(uv_timer_t));
    timer->data = req->data;
    rv = uv_timer_init(req->loop, timer);

    rv = uv_timer_start(timer, timer_cb, 2000, 0);
}

void
timer_cb(uv_timer_t* handle){
    int rv;

    uv_write_t* req = malloc(sizeof(uv_write_t));
    uv_buf_t* buf = malloc(sizeof(uv_buf_t));
    memset(buf, 0, sizeof(uv_buf_t));
    buf->base = malloc(4*1024);
    strcpy(buf->base, "Saludos");
    buf->len = 4*1024;

    req->data = handle->data;
    ESP_LOGI("timer_cb", "uv_write");
    rv = uv_write(req, handle->data, buf, 1, write_cb);
    if(rv != 0){
        ESP_LOGE("timer_cb","Error while trying to write");
    }

    uv_timer_stop(handle);
}

void
connection_cb(uv_stream_t* server, int status){
    int rv;
    ESP_LOGI("CONNECTION_CB", "Connection callback has been called with status = %d", status);

    // If listen has failed, try again
    if(status != 0){
        tcp_server->data = malloc(sizeof(uv_stream_t));

        rv = uv_listen((uv_stream_t*)tcp_server, 1, connection_cb);
        if(rv != 0){
            ESP_LOGE("UV_LISTEN", "Error in uv_listen in main_tcp");
        }
        return;
    }

    ESP_LOGI("CONNECTION_CB", "uv_accept");
    rv = uv_accept(server, tcp_server->data);
    if(rv != 0){
        ESP_LOGE("CONNECTION_CB", "Error while trying to accept");
    }

    uv_timer_t* timer = malloc(sizeof(uv_timer_t));
    timer->data = tcp_server->data;
    rv = uv_timer_init(server->loop, timer);

    rv = uv_timer_start(timer, timer_cb, 2000, 0);
}

void
startClientCb(uv_timer_t* timer){
    tcp_client = malloc(sizeof(uv_tcp_t));
    int rv;
    uv_loop_t* loop = timer->data;

    rv = uv_tcp_init(loop, tcp_client);
    if(rv != 0){
        ESP_LOGE("startClientCb", "Error in first uv_tcp_init in main_tcp");
    }

    ESP_LOGI("startClientCb", "First uv_tcp_init success");

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(50001);
    client_addr.sin_addr.s_addr = inet_addr(LOCALIP);

    rv = uv_tcp_bind(tcp_client, (struct sockaddr*)&client_addr, 0);
    if(rv != 0){
        ESP_LOGE("startClientCb", "Error in uv_tcp_bind in main_tcp");
    }

    ESP_LOGI("startClientCb", "uv_tcp_bind success");

    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000);
    server_addr.sin_addr.s_addr = inet_addr(CLIENTIP);

    rv = uv_tcp_connect(req, tcp_client, (struct sockaddr*)&server_addr, connect_cb);
    if(rv != 0){
        ESP_LOGE("startClientCb", "Error in uv_tcp_connect in main_tcp");
    }

    ESP_LOGI("startClientCb", "uv_tcp_connect success"); 

    uv_timer_stop(timer);
}

void
main_tcp(void* ignore){ 

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_tcp");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_tcp");

    // // Init server side
    tcp_server = malloc(sizeof(uv_tcp_t));

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

    tcp_server->data = malloc(sizeof(uv_stream_t));

    rv = uv_listen((uv_stream_t*)tcp_server, 1, connection_cb);
    if(rv != 0){
        ESP_LOGE("UV_LISTEN", "Error in uv_listen in main_tcp");
    }

    ESP_LOGI("UV_LISTEN", "uv_listen success");

    // Init client side
    uv_timer_t* timer_client = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, timer_client);
    timer_client->data = loop;
    uv_timer_start(timer_client, startClientCb, 1000, 0);

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_tcp");
    }

    vTaskDelete(NULL);
}

// FS test

#define BUTTON_NEWFILEDIR1  22
#define BUTTON_NEWFILEDIR2  19
#define BUTTON_INFO         18

// iniciar un boton para cada grupo de instrucciones
void
newFileDir1 (uv_signal_t* handle, int signum){
    int rv;
    FIL file;
    // FILE* file2;
    FRESULT fr;
    uv_fs_t req;
    const char* path = "dir1/example.txt";

    ESP_LOGI("NEWFILEDIR1", "f_mkdir");
    fr = f_mkdir("dir1");
    if((fr != FR_EXIST) && (fr != FR_OK)){
        ESP_LOGE("NEWFILEDIR1", "f_mkdir : %d", fr);
    }
    ESP_LOGI("NEWFILEDIR1", "f_mkdir successful");
    
    ESP_LOGI("NEWFILEDIR1", "uv_fs_open");
    file = uv_fs_open(NULL, &req, path, UV_FS_O_RDWR, 0, NULL);

    uv_buf_t bufs;
    bufs.base = malloc(sizeof("Mensaje de prueba\n"));
    memset(bufs.base, 0, sizeof("Mensaje de prueba\n"));
    strcpy(bufs.base, "Mensaje de prueba\n");
    bufs.len = sizeof("Mensaje de prueba\n");

    ESP_LOGI("NEWFILEDIR1", "uv_fs_write");
    rv = uv_fs_write(NULL, &req, file, &bufs, 1, 0, NULL);
    if(rv != sizeof("Mensaje de prueba\n")){
        ESP_LOGE("NEWFILEDIR1", "uv_fs_write");
    }

    free(bufs.base);

    rv = uv_fs_fsync(NULL, &req, file, NULL);
    if(rv != 0){
        ESP_LOGE("NEWFILEDIR1", "uv_fs_fsync");
    }

    ESP_LOGI("NEWFILEDIR1", "uv_fs_close");
    rv = uv_fs_close(NULL, &req, file, NULL);
    if(rv != 0){
        ESP_LOGE("NEWFILEDIR1", "uv_fs_close");
    }
}

void
newFile2Dir1 (uv_signal_t* handle, int signum){
    int rv;
    FIL file;
    FRESULT fr;
    uv_fs_t req;
    const char* path = "dir1/example2.txt";

    ESP_LOGI("newFile2Dir1", "f_mkdir");
    fr = f_mkdir("dir1");
    if((fr != FR_EXIST) && (fr != FR_OK)){
        ESP_LOGE("newFile2Dir1", "f_mkdir : %d", fr);
    }
    ESP_LOGI("newFile2Dir1", "f_mkdir successful");
    
    ESP_LOGI("newFile2Dir1", "uv_fs_open");
    file = uv_fs_open(NULL, &req, path, UV_FS_O_RDWR, 0, NULL);

    uv_buf_t bufs;
    bufs.base = malloc(sizeof("Mensaje de prueba\n"));
    memset(bufs.base, 0, sizeof("Mensaje de prueba\n"));
    strcpy(bufs.base, "Mensaje de prueba\n");
    bufs.len = sizeof("Mensaje de prueba\n");

    ESP_LOGI("NEWFILEDIR2", "uv_fs_write");
    rv = uv_fs_write(NULL, &req, file, &bufs, 1, 0, NULL);
    if(rv != sizeof("Mensaje de prueba\n")){
        ESP_LOGE("NEWFILEDIR2", "uv_fs_write");
    }

    free(bufs.base);

    ESP_LOGI("NEWFILEDIR2", "uv_fs_close");
    rv = uv_fs_close(NULL, &req, file, NULL);
    if(rv != 0){
        ESP_LOGE("NEWFILEDIR2", "uv_fs_close");
    }

    path = "dir1/example.txt";
    memset(&file,0, sizeof(FIL));
    memset(&req,0, sizeof(uv_fs_t));

    ESP_LOGI("newFile2Dir1", "uv_fs_open");
    file = uv_fs_open(NULL, &req, path, UV_FS_O_RDWR, 0, NULL);

    memset(&bufs, 0, sizeof(uv_buf_t));
    bufs.base = malloc(sizeof("Mensaje de prueba2\n"));
    memset(bufs.base, 0, sizeof("Mensaje de prueba2\n"));
    strcpy(bufs.base, "Mensaje de prueba2\n");
    bufs.len = sizeof("Mensaje de prueba2\n");

    ESP_LOGI("newFile2Dir1", "uv_fs_write");
    rv = uv_fs_write(NULL, &req, file, &bufs, 1, 0, NULL);
    if(rv != sizeof("Mensaje de prueba2\n")){
        ESP_LOGE("newFile2Dir1", "uv_fs_write");
    }
    free(bufs.base);

    ESP_LOGI("newFile2Dir1", "uv_fs_close");
    rv = uv_fs_close(NULL, &req, file, NULL);
    if(rv != 0){
        ESP_LOGE("newFile2Dir1", "uv_fs_close");
    }
}

void
info(uv_signal_t* handle, int signum){
    int rv;
    int n;
    FIL file;
    uv_fs_t req;
    uv_dirent_t ent;
    const char* path1 = "dir1";

    ESP_LOGI("INFO", "uv_fs_scandir");
    n = uv_fs_scandir(NULL, &req, path1, 0, NULL);
    ESP_LOGI("INFO", "Numero de archivos %d", n);

    ESP_LOGI("INFO", "print results");
    uv_buf_t buf;

    for(int i = 0; i < n; i++){
        ESP_LOGI("INFO", "uv_fs_scandir_next");
        rv = uv_fs_scandir_next(&req, &ent);
        if(rv != 0){
            ESP_LOGE("INFO", "uv_fs_scandir_next %d", rv);
        }
        ESP_LOGI("INFO", "File name is %s", ent.name);
        ESP_LOGI("INFO", "uv_fs_open");
        file = uv_fs_open(NULL, &req, ent.name, UV_FS_O_RDONLY, 0, NULL);
        ESP_LOGI("INFO", "uv_fs_stat");
        rv = uv_fs_stat(NULL, &req, ent.name, NULL);
        ESP_LOGI("INFO", "uv_fs_read, file size is %d", req.statbuf.st_size);
        buf.len = req.statbuf.st_size;
        buf.base = malloc(buf.len);
        ESP_LOGI("INFO", "uv_fs_read");
        rv = uv_fs_read(NULL, &req, file, &buf, 1, 0, NULL);

        for(int i = 0; i < req.statbuf.st_size; i++){
            ESP_LOGI("INFO", "%c", buf.base[i]);
        }
        // ESP_LOGI("INFO", "%s %d %d", buf, strlen(buf), br);

        ESP_LOGI("INFO", "uv_fs_close");
        rv = uv_fs_close(NULL, &req, file, NULL);
        if(rv != 0){
            ESP_LOGE("INFO", "uv_fs_close");
        }
        free(buf.base);
    }
}

void
main_fs(void* ignore){
    // Configure GPIO
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL<<BUTTON_NEWFILEDIR1)|(1ULL<<BUTTON_NEWFILEDIR2) | (1ULL<<BUTTON_INFO));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("LOOP_INIT","Error durante la inicializacion en main_fs");
    }

    ESP_LOGI("LOOP_INIT", "Loop inicializado en main_fs");

    // Init signal handle
    uv_signal_t* signal_info = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_dir1 = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_dir2 = malloc(sizeof(uv_signal_t));

    rv = uv_signal_init(loop, signal_info);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la primera inicializacion en main_fs");
    }

    ESP_LOGI("SIGNAL_INIT", "Primer signal inicializado en main_fs");

    rv = uv_signal_start(signal_info, info, BUTTON_INFO);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante primer uv_signal_start en main_fs");
    }

    ESP_LOGI("SIGNAL_START", "Primer uv_signal_start en main_fs");

    rv = uv_signal_init(loop, signal_dir1);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la primera inicializacion en main_fs");
    }

    ESP_LOGI("SIGNAL_INIT", "Primer signal inicializado en main_fs");

    // Aqui hay un error
    rv = uv_signal_start(signal_dir1, newFileDir1, BUTTON_NEWFILEDIR1);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante primer uv_signal_start en main_fs");
    }

    ESP_LOGI("SIGNAL_START", "Primer uv_signal_start en main_fs");

    rv = uv_signal_init(loop, signal_dir2);
    if(rv != 0){
        ESP_LOGE("SIGNAL_INIT","Error durante la primera inicializacion en main_fs");
    }

    ESP_LOGI("SIGNAL_INIT", "Primer signal inicializado en main_fs");

    rv = uv_signal_start(signal_dir2, newFile2Dir1, BUTTON_NEWFILEDIR2);
    if(rv != 0){
        ESP_LOGE("SIGNAL_START","Error durante primer uv_signal_start en main_fs");
    }

    ESP_LOGI("SIGNAL_START", "Primer uv_signal_start en main_fs");

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("UV_RUN", "Error in uv_run in main_fs");
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
    wifi_init(LOCALIP);

    // Mount FAT-VFS
    ESP_LOGI("APP_MAIN", "Mounting FAT filesystem");

    esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "fatvfs");
    esp_err_t err = esp_partition_erase_range(partition,0, 1048576);
    if (err != ESP_OK) {
        ESP_LOGE("APP_MAIN", "Failed to format FATFS (%s)", esp_err_to_name(err));
        return;
    }

    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 5,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    err = esp_vfs_fat_spiflash_mount("/spiflash", "fatvfs", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE("APP_MAIN", "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }

    ESP_LOGI("APP_MAIN", "FAT filesystem mounted succesfully");

    // xTaskCreate(main_signal, "startup", 16384, NULL, 5, NULL);
    // xTaskCreate(main_timer, "startup", 16384, NULL, 5, NULL);
    xTaskCreate(main_tcp, "startup", 16384, NULL, 5, NULL);
    // xTaskCreate(main_fs, "startup", 32768, NULL, 5, NULL);
}