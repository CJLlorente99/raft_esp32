#include "uv.h"

int
uv_fs_close(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb){
    // f_close()
    return 0;
}

int
uv_fs_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, int mode, uv_fs_cb cb){
    // f_open()
    return 0;
}

int
uv_fs_write(uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, uv_fs_cb cb){
    // f_write()
    return 0;
}

int
uv_fs_scandir(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, uv_fs_cb cb){
    // f_findfirst()
    return 0;
}

int
uv_fs_scandir_next(uv_fs_t* req, uv_dirent_t* ent){
    // f_findnext
    return 0;
}

int
uv_fs_stat(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb){
    // f_stat()
    return 0;
}

int
uv_fs_rename(uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, uv_fs_cb cb){
    // f_rename()
    return 0;
}

int
uv_fs_fsync(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb){
    // f_sync()
    return 0;
}

int
uv_fs_datasync(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb){
    // f_sync()
    return 0;
}

int
uv_fs_ftruncate(uv_loop_t* loop, uv_fs_t* req, uv_file file, int64_t offset, uv_fs_cb cb){
    // f_truncate()
    return 0;
}

int
uv_fs_unlink(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb){
    // f_unlink()
    return 0;
}