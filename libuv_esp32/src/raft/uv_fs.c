#include "uv_fs.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "err.h"
#include "heap.h"
#include "uv_os.h"

int UvFsCheckDir(const char *dir, char *errmsg)
{
    struct uv_fs_s req;
    int rv;

    /* Make sure we have a directory we can write into. */
    rv = uv_fs_stat(NULL, &req, dir, NULL);
    if (rv != 0) {
        switch (rv) {
            case UV_ENOENT:
                ErrMsgPrintf((char *)errmsg, "directory '%s' does not exist",
                             dir);
                return RAFT_NOTFOUND;
            case UV_EACCES:
                ErrMsgPrintf((char *)errmsg, "can't access directory '%s'",
                             dir);
                return RAFT_UNAUTHORIZED;
            case UV_ENOTDIR:
                ErrMsgPrintf((char *)errmsg, "path '%s' is not a directory",
                             dir);
                return RAFT_INVALID;
        }
        ErrMsgPrintf((char *)errmsg, "can't stat '%s': %d", dir,
                     rv);
        return RAFT_IOERR;
    }

    if (!(req.statbuf.st_mode & AM_DIR)) {
        ErrMsgPrintf((char *)errmsg, "path '%s' is not a directory", dir);
        return RAFT_INVALID;
    }

    if (false) {
        ErrMsgPrintf((char *)errmsg, "directory '%s' is not writable", dir);
        return RAFT_INVALID;
    }

    return 0;
}

int UvFsSyncDir(const char *dir, char *errmsg)
{
    uv_file fd;
    int rv;
    rv = UvOsOpen(dir, UV_FS_O_RDONLY | UV_FS_O_DIRECTORY, 0, &fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "open directory", rv);
        return RAFT_IOERR;
    }
    rv = UvOsFsync(fd);
    UvOsClose(fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "fsync directory", rv);
        return RAFT_IOERR;
    }
    return 0;
}

int UvFsFileExists(const char *dir,
                   const char *filename,
                   bool *exists,
                   char *errmsg)
{
    uv_stat_t sb;
    char path[UV__PATH_SZ];
    int rv;

    UvOsJoin(dir, filename, path);

    rv = UvOsStat(path, &sb);
    if (rv != 0) {
        if (rv == UV_ENOENT) {
            *exists = false;
            goto out;
        }
        UvOsErrMsg(errmsg, "stat", rv);
        return RAFT_IOERR;
    }

    *exists = true;

out:
    return 0;
}

/* Get the size of the given file. */
int UvFsFileSize(const char *dir,
                 const char *filename,
                 off_t *size,
                 char *errmsg)
{
    uv_stat_t sb;
    char path[UV__PATH_SZ];
    int rv;

    UvOsJoin(dir, filename, path);

    rv = UvOsStat(path, &sb);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "stat", rv);
        return RAFT_IOERR;
    }
    *size = (off_t)sb.st_size;

    return 0;
}

int UvFsFileIsEmpty(const char *dir,
                    const char *filename,
                    bool *empty,
                    char *errmsg)
{
    off_t size;
    int rv;

    rv = UvFsFileSize(dir, filename, &size, errmsg);
    if (rv != 0) {
        return rv;
    }
    *empty = size == 0 ? true : false;
    return 0;
}

/* Open a file in a directory. */
static int uvFsOpenFile(const char *dir,
                        const char *filename,
                        int flags,
                        int mode,
                        uv_file *fd,
                        char *errmsg)
{
    char path[UV__PATH_SZ];
    int rv;
    UvOsJoin(dir, filename, path);
    rv = UvOsOpen(path, flags, mode, fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "open", rv);
        return RAFT_IOERR;
    }
    return 0;
}

int UvFsOpenFileForReading(const char *dir,
                           const char *filename,
                           uv_file *fd,
                           char *errmsg)
{
    char path[UV__PATH_SZ];
    int flags = O_RDONLY;

    UvOsJoin(dir, filename, path);

    return uvFsOpenFile(dir, filename, flags, 0, fd, errmsg);
}

int UvFsAllocateFile(const char *dir,
                     const char *filename,
                     size_t size,
                     uv_file *fd,
                     char *errmsg)
{
    char path[UV__PATH_SZ];
    int flags = O_WRONLY | O_CREAT | O_EXCL; /* Common open flags */
    int rv = 0;

    UvOsJoin(dir, filename, path);

    /* TODO: use RWF_DSYNC instead, if available. */

    rv = uvFsOpenFile(dir, filename, flags, S_IRUSR | S_IWUSR, fd, errmsg);
    if (rv != 0) {
        goto err;
    }

    /* Allocate the desired size. */
    rv = UvOsFallocate(*fd, 0, (off_t)size);
    if (rv != 0) {
        switch (rv) {
            case FR_DENIED:
                ErrMsgPrintf(errmsg, "not enough space to allocate %zu bytes",
                             size);
                rv = RAFT_NOSPACE;
                break;
            default:
                UvOsErrMsg(errmsg, "f_expand", rv);
                rv = RAFT_IOERR;
                break;
        }
        goto err_after_open;
    }

    return 0;

err_after_open:
    UvOsClose(*fd);
    UvOsUnlink(path);
err:
    assert(rv != 0);
    return rv;
}

static int uvFsWriteFile(const char *dir,
                         const char *filename,
                         int flags,
                         struct raft_buffer *bufs,
                         unsigned n_bufs,
                         char *errmsg)
{
    uv_file fd;
    int rv;
    size_t size;
    unsigned i;
    size = 0;
    for (i = 0; i < n_bufs; i++) {
        size += bufs[i].len;
    }
    rv = uvFsOpenFile(dir, filename, flags, S_IRUSR | S_IWUSR, &fd, errmsg);
    if (rv != 0) {
        goto err;
    }
    rv = UvOsWrite(fd, (const uv_buf_t *)bufs, n_bufs, 0);
    if (rv != (int)(size)) {
        if (rv < 0) {
            UvOsErrMsg(errmsg, "write", rv);
        } else {
            ErrMsgPrintf(errmsg, "short write: %d only bytes written", rv);
        }
        goto err_after_file_open;
    }
    rv = UvOsFsync(fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "fsync", rv);
        goto err_after_file_open;
    }
    rv = UvOsClose(fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "close", rv);
        goto err;
    }
    return 0;

err_after_file_open:
    UvOsClose(fd);
err:
    return rv;
}

int UvFsMakeFile(const char *dir,
                 const char *filename,
                 struct raft_buffer *bufs,
                 unsigned n_bufs,
                 char *errmsg)
{
    int flags = UV_FS_O_WRONLY | UV_FS_O_CREAT | UV_FS_O_EXCL;
    return uvFsWriteFile(dir, filename, flags, bufs, n_bufs, errmsg);
}

int UvFsMakeOrOverwriteFile(const char *dir,
                            const char *filename,
                            const struct raft_buffer *buf,
                            char *errmsg)
{
    char path[UV__PATH_SZ];
    int flags = UV_FS_O_WRONLY;
    int mode = 0;
    bool exists = true;
    uv_file fd;
    int rv;

    UvOsJoin(dir, filename, path);

open:
    rv = UvOsOpen(path, flags, mode, &fd);
    if (rv != 0) {
        if (rv == UV_ENOENT && !(flags & UV_FS_O_CREAT)) {
            exists = false;
            flags |= UV_FS_O_CREAT;
            mode = S_IRUSR | S_IWUSR;
            goto open;
        }
        goto err;
    }

    rv = UvOsWrite(fd, (const uv_buf_t *)buf, 1, 0);
    if (rv != (int)(buf->len)) {
        if (rv < 0) {
            UvOsErrMsg(errmsg, "write", rv);
        } else {
            ErrMsgPrintf(errmsg, "short write: %d only bytes written", rv);
        }
        goto err_after_file_open;
    }

    if (exists) {
        rv = UvOsFdatasync(fd);
        if (rv != 0) {
            UvOsErrMsg(errmsg, "fsync", rv);
            goto err_after_file_open;
        }
    } else {
        rv = UvOsFsync(fd);
        if (rv != 0) {
            UvOsErrMsg(errmsg, "fsync", rv);
            goto err_after_file_open;
        }
    }

    rv = UvOsClose(fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "close", rv);
        goto err;
    }

    if (!exists) {
        rv = UvFsSyncDir(dir, errmsg);
        if (rv != 0) {
            goto err;
        }
    }

    return 0;

err_after_file_open:
    UvOsClose(fd);
err:
    return RAFT_IOERR;
}

bool UvFsIsAtEof(uv_file fd)
{
    return f_eof(&fd) != 0;           /* Compare current offset and size */
}

int UvFsReadInto(uv_file fd, struct raft_buffer *buf, char *errmsg)
{
    int rv;
    uv_fs_t req;
    rv = uv_fs_read(NULL, &req, fd, (uv_buf_t*)buf, 1, 0, NULL);
    if (rv == -1) {
        UvOsErrMsg(errmsg, "read", -errno);
        return RAFT_IOERR;
    }
    assert(rv >= 0);
    if ((size_t)rv < buf->len) {
        ErrMsgPrintf(errmsg, "short read: %d bytes instead of %zu", rv,
                     buf->len);
        return RAFT_IOERR;
    }
    return 0;
}

int UvFsReadFile(const char *dir,
                 const char *filename,
                 struct raft_buffer *buf,
                 char *errmsg)
{
    uv_stat_t sb;
    char path[UV__PATH_SZ];
    uv_file fd;
    int rv;

    UvOsJoin(dir, filename, path);

    rv = UvOsStat(path, &sb);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "stat", rv);
        rv = RAFT_IOERR;
        goto err;
    }

    rv = uvFsOpenFile(dir, filename, O_RDONLY, 0, &fd, errmsg);
    if (rv != 0) {
        goto err;
    }

    buf->len = (size_t)sb.st_size;
    buf->base = HeapMalloc(buf->len);
    if (buf->base == NULL) {
        ErrMsgOom(errmsg);
        rv = RAFT_NOMEM;
        goto err_after_open;
    }

    rv = UvFsReadInto(fd, buf, errmsg);
    if (rv != 0) {
        goto err_after_buf_alloc;
    }

    UvOsClose(fd);

    return 0;

err_after_buf_alloc:
    HeapFree(buf->base);
err_after_open:
    UvOsClose(fd);
err:
    return rv;
}

int UvFsReadFileInto(const char *dir,
                     const char *filename,
                     struct raft_buffer *buf,
                     char *errmsg)
{
    char path[UV__PATH_SZ];
    uv_file fd;
    int rv;

    UvOsJoin(dir, filename, path);

    rv = uvFsOpenFile(dir, filename, O_RDONLY, 0, &fd, errmsg);
    if (rv != 0) {
        goto err;
    }

    rv = UvFsReadInto(fd, buf, errmsg);
    if (rv != 0) {
        goto err_after_open;
    }

    UvOsClose(fd);

    return 0;

err_after_open:
    UvOsClose(fd);
err:
    return rv;
}

int UvFsRemoveFile(const char *dir, const char *filename, char *errmsg)
{
    char path[UV__PATH_SZ];
    int rv;
    UvOsJoin(dir, filename, path);
    rv = UvOsUnlink(path);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "unlink", rv);
        return RAFT_IOERR;
    }
    return 0;
}

int UvFsTruncateAndRenameFile(const char *dir,
                              size_t size,
                              const char *filename1,
                              const char *filename2,
                              char *errmsg)
{
    char path1[UV__PATH_SZ];
    char path2[UV__PATH_SZ];
    uv_file fd;
    int rv;

    UvOsJoin(dir, filename1, path1);
    UvOsJoin(dir, filename2, path2);

    /* Truncate and rename. */
    rv = UvOsOpen(path1, UV_FS_O_RDWR, 0, &fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "open", rv);
        goto err;
    }
    rv = UvOsTruncate(fd, (off_t)size);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "truncate", rv);
        goto err_after_open;
    }
    rv = UvOsFsync(fd);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "fsync", rv);
        goto err_after_open;
    }
    UvOsClose(fd);

    rv = UvOsRename(path1, path2);
    if (rv != 0) {
        UvOsErrMsg(errmsg, "rename", rv);
        goto err;
    }

    return 0;

err_after_open:
    UvOsClose(fd);
err:
    return RAFT_IOERR;
}

int UvFsProbeCapabilities(const char *dir,
                          size_t *direct,
                          bool *async,
                          char *errmsg)
{
    *direct = 4096;
    *async = false;

    return 0;
}
