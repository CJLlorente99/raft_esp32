#include "uv_os.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <uv.h>

#include "assert.h"
#include "err.h"
#include "syscall.h"

/* Default permissions when creating a directory. */
#define DEFAULT_DIR_PERM 0700

int UvOsOpen(const char *path, int flags, int mode, uv_file *fd)
{
    struct uv_fs_s req;
    FIL rv;
    rv = uv_fs_open(NULL, &req, path, flags, mode, NULL);
    *fd = rv;
    return 0;
}

int UvOsClose(uv_file fd)
{
    struct uv_fs_s req;
    return uv_fs_close(NULL, &req, fd, NULL);
}

int UvOsFallocate(uv_file fd, off_t offset, off_t len)
{
    // TODO (maybe use f_expand)
    FRESULT fr;
    fr = f_expand(&fd, len, 1);
    if (fr != 0) {
        ESP_LOGE("UvOsFallocate", "%d", fr);
    }
    return fr;
}

int UvOsTruncate(uv_file fd, off_t offset)
{
    struct uv_fs_s req;
    return uv_fs_ftruncate(NULL, &req, fd, offset, NULL);
}

int UvOsFsync(uv_file fd)
{
    struct uv_fs_s req;
    return uv_fs_fsync(NULL, &req, fd, NULL);
}

int UvOsFdatasync(uv_file fd)
{
    struct uv_fs_s req;
    return uv_fs_fsync(NULL, &req, fd, NULL);
}

int UvOsStat(const char *path, uv_stat_t *sb)
{
    struct uv_fs_s req;
    int rv;
    rv = uv_fs_stat(NULL, &req, path, NULL);
    if (rv != 0) {
        return rv;
    }
    memcpy(sb, &req.statbuf, sizeof *sb);
    return 0;
}

int UvOsWrite(uv_file fd,
              const uv_buf_t bufs[],
              unsigned int nbufs,
              int64_t offset)
{
    struct uv_fs_s req;
    return uv_fs_write(NULL, &req, fd, bufs, nbufs, offset, NULL);
}

int UvOsUnlink(const char *path)
{
    struct uv_fs_s req;
    return uv_fs_unlink(NULL, &req, path, NULL);
}

int UvOsRename(const char *path1, const char *path2)
{
    struct uv_fs_s req;
    return uv_fs_rename(NULL, &req, path1, path2, NULL);
}

void UvOsJoin(const char *dir, const char *filename, char *path)
{
    assert(UV__DIR_HAS_VALID_LEN(dir));
    assert(UV__FILENAME_HAS_VALID_LEN(filename));
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, filename);
}
