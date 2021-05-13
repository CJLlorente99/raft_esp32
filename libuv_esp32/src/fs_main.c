#include "uv.h"

#define BUTTON_NEWFILEDIR1  22
#define BUTTON_NEWFILEDIR2  19
#define BUTTON_INFO         18

/* Start callbacks that should test various functionalities */
void
newFile_callback (uv_signal_t* handle, int signum){
    int rv;
    FIL file;
    uv_fs_t req;
    const char* path = "dir1/a.txt";
    
    file = uv_fs_open(NULL, &req, path, UV_FS_O_RDWR, 0, NULL);

    uv_buf_t bufs;
    bufs.base = malloc(sizeof("Mensaje de prueba\n"));
    strcpy(bufs.base, "Mensaje de prueba\n");
    bufs.len = sizeof("Mensaje de prueba\n");

    rv = uv_fs_write(NULL, &req, &file, &bufs, 1, 0, NULL);
    if(rv != sizeof("Mensaje de prueba\n")){
        ESP_LOGE("newFile_callback", "uv_fs_write");
    }
    // UINT bw;
    // f_write(&file, bufs.base, bufs.len, &bw);    

    rv = uv_fs_fsync(NULL, &req, file, NULL);
    if(rv != 0){
        ESP_LOGE("newFile_callback", "uv_fs_fsync");
    }

    free(bufs.base);

    rv = uv_fs_close(NULL, &req, file, NULL);
    if(rv != 0){
        ESP_LOGE("newFile_callback", "uv_fs_close");
    }

    // rv = uv_fs_stat(NULL, &req, path, NULL);
    // ESP_LOGI("", "size is %d, rv is %d", req.statbuf.st_size, rv);
}

void
changeFileName_callback (uv_signal_t* handle, int signum){
    uv_fs_t req;
    const char* oldpath = "dir1/a.txt";
    const char* path = "dir1/b.txt";

    uv_fs_rename(NULL, &req, oldpath, path, NULL);
}

void
info_callback(uv_signal_t* handle, int signum){
    int rv;
    int n = 0;
    FIL file;
    uv_fs_t req;
    memset(&req, 0, sizeof(req));
    uv_dirent_t ent;
    memset(&ent, 0, sizeof(uv_dirent_t));
    const char* path1 = "dir1";

    n = uv_fs_scandir(NULL, &req, path1, 0, NULL);
    ESP_LOGI("info_callback", "Numero de archivos %d", n);

    uv_buf_t buf;

    rv = uv_fs_stat(NULL, &req, "dir1", NULL);
    ESP_LOGI("info_callback", "Mode is %d, should be %d", req.statbuf.st_mode, AM_DIR);

    rv = uv_fs_stat(NULL, &req, "dir1/EXAMPLE1.TEXT", NULL);
    ESP_LOGI("info_callback", "rv is %d, should be %d", rv, UV_ENOENT);

    for(int i = 0; i < n; i++){
        rv = uv_fs_scandir_next(&req, &ent);
        if(rv == EOF){
            break;
        }
        if(rv != 0){
            ESP_LOGE("info_callback", "uv_fs_scandir_next %d", rv);
        }
        ESP_LOGI("info_callback", "File name is %s", ent.name);

        char path[255];
        strcpy(path,"dir1/");
        strcat(path, ent.name);

        ESP_LOGI("info_callback", "Path is %s", path);
        file = uv_fs_open(NULL, &req, path, UV_FS_O_RDONLY, 0, NULL);
        rv = uv_fs_stat(NULL, &req, path, NULL);
        ESP_LOGI("info_callback", "file size is %d, mode is %d, rv is %d", req.statbuf.st_size, req.statbuf.st_mode, rv);

        if(req.statbuf.st_size > 0){
            buf.len = req.statbuf.st_size;
            buf.base = malloc(buf.len);

            rv = uv_fs_read(NULL, &req, file, &buf, 1, 0, NULL);

            ESP_LOGI("info_callback", "%s", buf.base);

            free(buf.base);
        }

        rv = uv_fs_close(NULL, &req, file, NULL);
        if(rv != 0){
            ESP_LOGE("info_callback", "uv_fs_close");
        }
    }
}

void
main_fs(void* ignore){
    /* Configure GPIO */
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL<<BUTTON_NEWFILEDIR1)|(1ULL<<BUTTON_NEWFILEDIR2) | (1ULL<<BUTTON_INFO));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf); 

    f_mkdir("dir1");

    /* Init loop */
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        ESP_LOGE("main_fs","Error durante la inicializacion en main_fs");
    }

    /* Init signal handle */
    uv_signal_t* signal_info = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_newFile = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_rename = malloc(sizeof(uv_signal_t));

    rv = uv_signal_init(loop, signal_info);
    if(rv != 0){
        ESP_LOGE("main_fs","uv_signal_init: signal_info");
    }

    rv = uv_signal_start(signal_info, info_callback, BUTTON_INFO);
    if(rv != 0){
        ESP_LOGE("main_fs","uv_signal_start: signal_info");
    }

    rv = uv_signal_init(loop, signal_rename);
    if(rv != 0){
        ESP_LOGE("main_fs","uv_signal_start: signal_rename");
    }

    rv = uv_signal_start(signal_rename, changeFileName_callback, BUTTON_NEWFILEDIR1);
    if(rv != 0){
        ESP_LOGE("main_fs","uv_signal_start: signal_rename");
    }

    rv = uv_signal_init(loop, signal_newFile);
    if(rv != 0){
        ESP_LOGE("main_fs","uv_signal_init: signal_newFile");
    }

    rv = uv_signal_start(signal_newFile, newFile_callback, BUTTON_NEWFILEDIR2);
    if(rv != 0){
        ESP_LOGE("main_fs","uv_signal_start: signal_newFile");
    }

    rv = uv_run(loop, UV_RUN_DEFAULT);
    if(rv != 0){
        ESP_LOGE("main_fs", "uv_run");
    }

    // FIL file;
    // UINT bw, br;
    // FILINFO fno;
    // char buf[64];
    // FRESULT fr;
    // f_mkdir("dir1");
    // f_open(&file, "dir1/0000000000016-0000000000016", FA_OPEN_ALWAYS|FA_READ|FA_WRITE);
    // f_write(&file, "Ejemplo", sizeof("Ejemplo"), &bw);
    // f_sync(&file);
    // f_close(&file);
    // fr = f_stat("dir1/0000000000016-0000000000016", &fno);
    // ESP_LOGI("", "name is %s, size is %u, mode is %u, fr is %d", fno.fname, fno.fsize, fno.fattrib, fr);
    // f_open(&file, "dir1/0000000000016-0000000000016", FA_READ);
    // f_read(&file, &buf, sizeof("Ejemplo"), &br);
    // f_close(&file);
    // ESP_LOGI("", "%s %d", buf, br);
    // fr = f_stat("dir1/0000000000016-0000000000016", &fno);
    // ESP_LOGI("", "name is %s, size is %u, mode is %u, fr is %d", fno.fname,fno.fsize, fno.fattrib, fr);

    // while(1){}

    vTaskDelete(NULL);
}