#include "uv.h"

void
main_limits(void* ignore){

    f_mkdir("dir1");

    uint32_t written = 0;
    uv_fs_t req;
    uv_file file;

    uv_buf_t buf[8];
    char* base = malloc(100*1024);
    memset(base, 1, 100*1024);

    for (int i = 0; i < 8; i++){
        buf[i].base = base;
        buf[i].len = 100*1024;
    }

    file = uv_fs_open(NULL, &req, "dir1/test.txt", UV_FS_O_RDWR, 0 , NULL);

    // while(1){
    //     written += uv_fs_write(NULL, &req, &file, &buf, 1, 0, NULL);
    //     uv_fs_fsync(NULL, &req, file, NULL);
    //     uv_fs_stat(NULL, &req, "dir1/test.txt", NULL);
    //     ESP_LOGI("main_fs", "total size is %u B, bytes written %u B", req.statbuf.st_size, written);
    // }

    ESP_LOGI("main_fs", "writing started %d", pdTICKS_TO_MS(xTaskGetTickCount()));
    while(1){
        written += uv_fs_write(NULL, &req, &file, buf, 8, 0, NULL);
        uv_fs_fsync(NULL, &req, file, NULL);
        uv_fs_stat(NULL, &req, "dir1/test.txt", NULL);
        if(req.statbuf.st_size == 995328){
            ESP_LOGI("main_fs", "writing finished %d", pdTICKS_TO_MS(xTaskGetTickCount()));
            break;
        }
    }

    vTaskDelete(NULL);
}