#include "uv.h"

/*
 *  CAREFULL
 *  ERRORS ARE ANALYZED IN RAFT TO GIVE INFO ABOUT
 *  CONSIDER "TRANSLATING" ERROR CODES
 *  FOR EXAMPLE: FR_NO_FILE (FATFS) = UV_ENOENT (SEE ERROR.C IN ORIGINAL LIBUV)
 *  AS WELL, CONSIDER RETURNING RV IN SOME CASES INSTEAD OF 1 OR 0 
*/

// virtual table for fs requests
static request_vtbl_t fs_req_vtbl = {
    .run = run_fs_req
};

// Seguramente uv_file debe de convertirse en un FIL
// In every call to these functions loop and cb are both set to NULL
// If offset is uint64, FF_FS_EXFAT is needed to be set to 1

int uv_fs_close(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_CLOSE", "Error during uv_insert in uv_fs_close");
            return 1;
        }
    }

    rv = f_close(&file);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_CLOSE", "Error in uv_fs_close. Code = %d", rv);
        return 1;
    }

    return 0;
}

FIL uv_fs_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, int mode, uv_fs_cb cb)
{
    // TODO
    // flags indicate mode
    // mode is always 0
    FIL* fp = malloc(sizeof(FIL));
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_OPEN", "Error during uv_insert in uv_fs_open");
            memset(fp, 0, sizeof(FIL));
            return *fp;
        }
    }

    rv = f_open(fp, path, flags);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_OPEN", "Error in uv_fs_open. Code = %d", rv);
        memset(fp, 0, sizeof(FIL));
        return *fp;
    }

    return *fp;
}

int uv_fs_write(uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;
    UINT bw;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_OPEN", "Error during uv_insert in uv_fs_open");
            return 1;
        }
    }

    rv = f_lseek(&file, offset);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_WRITE", "Error in uv_fs_write. Code = %d", rv);
        return 1;
    }

    rv = f_write(&file, bufs, nbufs * sizeof(bufs[0]), &bw);
    if (rv != FR_OK || (bw < (nbufs*sizeof(bufs[0]))))
    {
        ESP_LOGE("UV_FS_WRITE", "Error in uv_fs_write. Code = %d", rv);
        return 1;
    }

    rv = f_lseek(&file, 0);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_WRITE", "Error in uv_fs_write. Code = %d", rv);
        return 1;
    }

    return 0;
}

int uv_fs_scandir(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, uv_fs_cb cb)
{
    // returns number of files in the directory
    // idea, use a counter and f_findfirst and f_findnext. 
    // When one of these returns fno->fname[] == null, no more files available
    return 0;
}

int uv_fs_scandir_next(uv_fs_t* req, uv_dirent_t* ent)
{
    // called after uv_fs_scandir
    // fills ent with the files present in req->dir (por ejemplo)
    // use f_findfirst and f_findnext to fill ent (containing the paths)
    return 0;
}

int uv_fs_stat(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb)
{
    // only used to know if a directory actually exists or to access to size
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;
    req = malloc(sizeof(uv_fs_t));
    uv_stat_t statbuf;
    FILINFO* fno = malloc(sizeof(FILINFO)); // is it necessary to malloc even if it is only
    // going to be used within this function

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_OPEN", "Error during uv_insert in uv_fs_open");
            return 1;
        }
    }

    rv = f_stat(path, fno);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_WRITE", "Error in uv_fs_write. Code = %d", rv);
        return 1;
    }

    statbuf.st_size = fno->fsize;
    req->statbuf = statbuf;

    return 0;
}

int uv_fs_rename(uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_RENAME", "Error during uv_insert in uv_fs_rename");
            return 1;
        }
    }

    rv = f_rename(path, new_path);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_RENAME", "Error in uv_fs_rename. Code = %d", rv);
        return 1;
    }
    return 0;
}

int uv_fs_fsync(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_FSYNC", "Error during uv_insert in uv_fs_rename");
            return 1;
        }
    }

    rv = f_sync(&file);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_FSYNC", "Error in uv_fs_rename. Code = %d", rv);
        return 1;
    }

    return 0;
}

int uv_fs_ftruncate(uv_loop_t* loop, uv_fs_t* req, uv_file file, int64_t offset, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_FTRUNCATE", "Error during uv_insert in uv_fs_ftruncate");
            return 1;
        }
    }

    rv = f_lseek(&file, offset);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_FTRUNCATE", "Error in uv_fs_ftruncate. Code = %d", rv);
        return 1;
    }

    rv = f_truncate(&file);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_FTRUNCATE", "Error in uv_fs_ftruncate. Code = %d", rv);
        return 1;
    }

    rv = f_lseek(&file, 0);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_FTRUNCATE", "Error in uv_fs_ftruncate. Code = %d", rv);
        return 1;
    }

    return 0;
}

int uv_fs_unlink(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req = malloc(sizeof(uv_fs_t));
        req->req.loop = loop;
        req->req.vtbl = &fs_req_vtbl;
        req->cb = cb;

        rv2 = uv_insert_request(loopFSM, (uv_request_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_UNLINK", "Error during uv_insert in uv_fs_unlink");
            return 1;
        }
    }

    rv = f_unlink(path);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_UNLINK", "Error in uv_fs_unlink. Code = %d", rv);
        return 1;
    }

    return 0;
}