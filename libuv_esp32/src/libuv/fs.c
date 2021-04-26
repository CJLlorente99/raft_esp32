#include "uv.h"

/*
 *  CAREFUL
 *  ERRORS ARE ANALYZED IN RAFT TO GIVE INFO ABOUT
 *  CONSIDER "TRANSLATING" ERROR CODES
 *  FOR EXAMPLE: FR_NO_FILE (FATFS) = UV_ENOENT (SEE ERROR.C IN ORIGINAL LIBUV)
 *  AS WELL, CONSIDER RETURNING RV IN SOME CASES INSTEAD OF 1 OR 0 
*/

/* Run function and vtbl */
void
run_fs_handle(uv_handle_t* handle){
    // NEVER USED IN RAFT
}

static handle_vtbl_t fs_handle_vtbl = {
    .run = run_fs_handle
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
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
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

    FIL fp;
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_OPEN", "Error during uv_insert in uv_fs_open");
            return fp;
        }
    }

    rv = f_open(&fp, path, flags|FA_OPEN_ALWAYS);
    if (rv)
    {
        ESP_LOGE("UV_FS_OPEN", "Error in uv_fs_open. Code = %x", rv);
        return fp;
    }

    return fp;
}

int uv_fs_write(uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;
    UINT bw;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_OPEN", "Error during uv_insert in uv_fs_open");
            return 1;
        }
    }

    FSIZE_t ind = f_tell(&file);
    f_lseek(&file, f_size(&file));

    for(int i = 0; i < nbufs; i++){
        ESP_LOGI("UV_FS_WRITE", "%s %u", bufs[i].base, (UINT)bufs[i].len);
        rv = f_write(&file, (BYTE*)bufs[i].base, bufs[i].len, &bw);
        if (rv != FR_OK || (bw != (bufs[i].len))){
            ESP_LOGE("UV_FS_WRITE", "Error in f_write uv_fs_write. Code = %d, bw = %u", rv, bw);
            return 1;
        }
        ESP_LOGI("UV_FS_WRITE", "%d bytes written, size is %d bytes", bw, f_size(&file));
    }

    rv = f_sync(&file);
    if(rv != FR_OK){
        ESP_LOGE("UV_FS_WRITE", "Error in f_sync in uv_fs_write. Code = %d",rv);
    }

    f_lseek(&file, ind);

    return bw;
}

int uv_fs_scandir(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, uv_fs_cb cb)
{
    // returns number of files in the directory
    // idea, use a counter and f_findfirst and f_findnext. 
    // When one of these returns fno->fname[] == null, no more files available
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;
    int fileCounter = 0;
    FF_DIR dp;
    FILINFO fno;

    free(req->path);
    req->path = malloc(sizeof(path));
    strcpy(req->path, path); 
    
    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_SCANDIR", "Error during uv_insert in uv_fs_open");
            return 1;
        }
    }

    rv = f_findfirst(&dp, &fno, path, "*");
    if(rv != FR_OK){
        ESP_LOGE("UV_FS_SCANDIR", "Error during f_findfirst in uv_fs_scandir. Code = %d", rv);
        return 0;
    }

    TCHAR* fname = fno.fname;
    while(fname[0] != NULL){
        fileCounter++;
        rv = f_findnext(&dp, &fno);
        if(rv != FR_OK){
            ESP_LOGE("UV_FS_SCANDIR", "Error during f_findnext in uv_fs_scandir. Code = %d", rv);
            return 0;
        }
        fname = fno.fname;
    }

    rv = f_closedir(&dp);
    if(rv != FR_OK){
        ESP_LOGE("UV_FS_SCANDIR", "Error during f_closedir in uv_fs_scandir. Code = %d", rv);
    }

    return fileCounter;
}

int uv_fs_scandir_next(uv_fs_t* req, uv_dirent_t* ent)
{
    // called after uv_fs_scandir
    // fills ent with the files present in req->dir (por ejemplo)
    // use f_findfirst and f_findnext to fill ent (containing the paths)
    FRESULT rv;
    FILINFO fno;
    TCHAR* fname;

    ent->type = UV_DIRENT_FILE;

    rv = f_findnext(&req->dp, &fno);
    if(rv == FR_INVALID_OBJECT){
        ESP_LOGI("UV_FS_SCANDIR_NEXT", "f_findfirst");
        rv = f_findfirst(&req->dp, &fno, req->path, "*");
        if(rv != FR_OK){
            ESP_LOGE("UV_FS_SCANDIR_NEXT", "Error during f_findfirst in uv_fs_scandir. Code = %d", rv);
            return 0;
        }
    }   else if(rv != FR_OK){
        ESP_LOGE("UV_FS_SCANDIR_NEXT", "Error during f_findnext in uv_fs_scandir. Code = %d", rv);
        return 0;
    }  

    fname = fno.fname;

    if(fname[0] == NULL){
        return EOF;
    }

    strcpy(ent->name, req->path);
    strcat(ent->name, "/");
    strcat(ent->name, fname);
    strcat(ent->name, "\0");

    return 0;
}

int uv_fs_stat(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb)
{
    // only used to know if a directory actually exists or to access to size
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;
    FIL fp;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_STAT", "Error during uv_insert in uv_fs_stat");
            return 1;
        }
    }

    // rv = f_open(&fp, path, UV_FS_O_RDONLY);
    // if(rv != FR_OK){
    //     ESP_LOGE("UV_FS_STAT", "Error during f_open in uv_fs_stat code %d",rv);
    //     return 1;
    // }

    FILINFO fno;

    rv = f_stat(path, &fno);
    if(rv != FR_OK){
        ESP_LOGE("UV_FS_STAT", "Error during f_stat in uv_fs_stat code %d",rv);
        return 1;
    }

    req->statbuf.st_size = fno.fattrib;
    req->statbuf.st_size = fno.fsize;

    // rv = f_close(&fp);
    // if(rv != FR_OK){
    //     ESP_LOGE("UV_FS_STAT", "Error during f_close in uv_fs_stat");
    //     return 1;
    // }

    return 0;
}

int uv_fs_rename(uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, uv_fs_cb cb)
{
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
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
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
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
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
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
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
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

/*  raft implementation does not use uv_fs_read and uses posix read. In this case
    use of uv_fs_read is needed */
//  raft only uses read to read into 1 buffer and no offset
int uv_fs_read(uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, uv_fs_cb cb){
    FRESULT rv;
    int rv2;
    loopFSM_t* loopFSM;

    uv_buf_t buf = bufs[0];
    uint32_t br;

    if(loop && cb){
        loopFSM = loop->loopFSM->user_data;
        req->loop = loop;
        req->req.vtbl = &fs_handle_vtbl;
        req->cb = cb;

        rv2 = uv_insert_handle(loopFSM, (uv_handle_t*)req);
        if (rv2 != 0)
        {
            ESP_LOGE("UV_FS_READ", "Error during uv_insert in uv_fs_read");
            return 1;
        }
    }

    if(f_eof(&file))
        return 0;

    rv = f_read(&file, buf.base, buf.len, &br);
    if (rv != FR_OK)
    {
        ESP_LOGE("UV_FS_READ", "Error in uv_fs_read. Code = %d", rv);
        return br;
    }

    return br;
}