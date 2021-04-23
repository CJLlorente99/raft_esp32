#include "uv.h"

/* Run function, vtbl and uv_listen */

/* Run listen function listen on a given socket. If success, then cb is called. Then handle is removed */
void
run_listen_handle(uv_handle_t* handle){
    int rv;
    uv_listen_t* listen_handle = (uv_listen_t*)handle;

    rv = listen(listen_handle->stream->socket, 1);
    listen_handle->status = rv;
    if(rv != 0){
        ESP_LOGE("run_listen_handle", "Error durign listen in run_listen_handle: errno %d", errno);
        return;
    }

    listen_handle->cb(listen_handle->stream, listen_handle->status);

    rv = uv_remove_handle(listen_handle->loop->loopFSM->user_data, handle);
    if(rv != 0){
        ESP_LOGE("run_listen_handle", "Unable to remove handle in run_listen_handle");
        return;
    }
}

static handle_vtbl_t listen_handle_vtbl = {
    .run = run_listen_handle
};

int
uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb){
    // backlog indicates number of connection to be queued (only backlog = 1 is used in raft)
    int rv;

    ((uv_tcp_t*)stream)->connection_cb = cb;

    uv_listen_t* req = malloc(sizeof(uv_listen_t));
    if(!req){
        ESP_LOGE("uv_listen", "Error during malloc in uv_listen");
        return 1;
    }

    req->req.loop = stream->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &listen_handle_vtbl;

    req->cb = cb;
    req->loop = stream->loop;
    req->status = 0;
    req->stream = stream;

    /* Add handle */
    rv = uv_insert_handle(stream->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_listen", "Error during uv_insert_request in uv_listen");
        return 1;
    }
        
    return 0;
}

/* Run function, vtbl and uv_accept */

/* Run accept tries to accept any possible incoming connection. Then removes handle */
void
run_accept_handle(uv_handle_t* handle){
    int rv;
    uv_accept_t* accept_handle = (uv_accept_t*)handle;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(accept_handle->server->socket, &readset);

    if(select(accept_handle->server->socket + 1, &readset, NULL, NULL, &tv)){
        rv = accept(accept_handle->server->socket, NULL, NULL);
        if(rv == -1){
            ESP_LOGE("run_accept_handle", "Error during accept in run_accept_handle: errno %d", errno);
            return;
        }
        char addr;
        struct sockaddr name;
        socklen_t len;
        getpeername(rv,&name,&len);
        inet_ntop(AF_INET, &(((struct sockaddr_in*)&name)->sin_addr), &addr, len);
        ESP_LOGI("RUN_TCP_ACCEPT", "accepting : %s", &addr);
        accept_handle->client->socket = rv;

        rv = uv_remove_handle(accept_handle->loop->loopFSM->user_data, handle);
        if(rv != 0){
            ESP_LOGE("run_accept_handle", "Unable to remove handle in run_accept_handle");
            return;
        }
    }
}

static handle_vtbl_t accept_handle_vtbl = {
    .run = run_accept_handle
};

int
uv_accept(uv_stream_t* server, uv_stream_t* client){
    int rv;
    
    uv_accept_t* req = malloc(sizeof(uv_accept_t));
    if(!req){
        ESP_LOGE("UV_ACCEPT", "Error during malloc in uv_accept");
        return 1;
    }

    req->req.loop = server->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &accept_handle_vtbl;

    req->client = client;
    req->loop = server->loop;
    req->server = server;

    /* Initialize client too */
    client->self.loop = server->loop;
    client->self.type = UV_STREAM;
    client->self.vtbl = NULL;

    client->alloc_cb = NULL;
    client->loop = server->loop;
    client->read_cb = NULL;
    client->server = server;
    client->socket = -1;
    client->type = UV_STREAM;

    /* Add handle */
    rv = uv_insert_handle(server->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_accept", "Error during uv_insert_request in uv_accept");
        return 1;
    }

    return 0;
}

/* Run function, vtbl and uv_read_start */

/* Run read_start function tries to allocate space for an incoming message (done only once per request). Then tries to read and calls read_cb everytiem something has been read. Handle is never removed until uv_read_stop is called. */
void
run_read_start_handle(uv_handle_t* handle){
    uv_read_start_t* read_start_handle = (uv_read_start_t*)handle;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(read_start_handle->stream->socket, &readset);

    if(read_start_handle->alloc_cb){
        uv_buf_t* buf = malloc(sizeof(uv_buf_t));
        buf->len = 4*1024;

        read_start_handle->buf = buf;
        read_start_handle->alloc_cb((uv_handle_t*) read_start_handle->stream, 4*1024, buf);
        read_start_handle->is_alloc = 1;

        read_start_handle->alloc_cb = NULL; // allocation should only be done once per read_start call
    }

    if(select(read_start_handle->stream->socket + 1, &readset, NULL, NULL, &tv) && read_start_handle->is_alloc){
        // CUIDADO, raft aumenta el mismo buf.base y reduce buf.len
        ssize_t nread = read(read_start_handle->stream->socket, read_start_handle->buf->base, read_start_handle->buf->len);
        read_start_handle->nread = nread;
        if(nread < 0){
            ESP_LOGE("run_read_start_handle", "Error during read in run_read_start_handle: errno %d", errno);
        }

        read_start_handle->read_cb(read_start_handle->stream, read_start_handle->nread, read_start_handle->buf);
        // DO NOT uv_remove the read start handle (uv_read_stop is the one doing that)
    }
}

static handle_vtbl_t read_start_handle_vtbl = {
    .run = run_read_start_handle
};

int
uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb){
    int rv;

    stream->alloc_cb = alloc_cb;
    stream->read_cb = read_cb;
    
    uv_read_start_t* req = malloc(sizeof(uv_read_start_t));
    if(!req){
        ESP_LOGE("UV_READ_START", "Error during malloc in uv_read_start");
        return 1;
    }

    req->req.loop = stream->loop;
    req->req.type = UV_READ_START;
    req->req.vtbl = &read_start_handle_vtbl;

    req->alloc_cb = alloc_cb;
    req->buf = NULL;
    req->is_alloc = 0;
    req->loop = stream->loop;
    req->nread = -1;
    req->read_cb = read_cb;
    req->stream = stream;

    rv = uv_insert_handle(stream->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("UV_READ_START","Error during uv_insert in uv_read_start");
        return 1;
    }

    return 0;
}

/* Run function, vtbl and uv_read_stop */

/* Run read_stop function ceases reading started by read_start. First identifies the correspondent read_start handle and then removes it. */
void
run_read_stop_handle(uv_handle_t* handle){
    uv_read_stop_t* read_stop_handle = (uv_read_stop_t*)handle;
    loopFSM_t* loop = read_stop_handle->loop->loopFSM->user_data;
    int rv;
    /* Stop the correspoding read_start_request */
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->type == UV_READ_START){
            uv_read_start_t* read_start_handle = (uv_read_start_t*)loop->active_handlers[i];
            if(read_start_handle->stream == read_stop_handle->stream){
                rv = uv_remove_handle(loop, loop->active_handlers[i]);
                if(rv != 0){
                    ESP_LOGE("run_read_stop_handle", "Error during uv_remove in run_read_stop_handle");
                    return;
                }
            } 
            
        }
    }
    rv = uv_remove_handle(loop, handle);
    if(rv != 0){
        ESP_LOGE("run_read_stop_handle", "Error during uv_remove in run_read_stop_handle");
        return;
    }
}

static handle_vtbl_t read_stop_handle_vtbl = {
    .run = run_read_stop_handle
};

int
uv_read_stop(uv_stream_t* stream){
    int rv;

    uv_read_stop_t* req = malloc(sizeof(uv_read_stop_t));
    if(!req){
        ESP_LOGE("UV_READ_START", "Error during malloc in uv_read_start");
        return 1;
    }

    req->req.loop = stream->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &read_stop_handle_vtbl;

    req->loop = stream->loop;
    req->stream = stream;

    rv = uv_insert_handle(stream->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_read_stop","Error during uv_insert in uv_read_stop");
        return 1;
    }

    return 0;
}

/* Run function, vtbl and uv_write */

/* Run write function tries to write the whole buffer. When done, write_cb is called. */
void
run_write_handle(uv_handle_t* handle){
    int rv;
    uv_write_t* write_handle = (uv_write_t*)handle;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set writeset;
    FD_ZERO(&writeset);
    FD_SET(write_handle->stream->socket, &writeset);

    if(select(write_handle->stream->socket + 1, NULL, &writeset, NULL, &tv)){
        rv = write(write_handle->stream->socket, write_handle->bufs->base, write_handle->nbufs * write_handle->bufs->len);
        write_handle->status = rv;
        if(rv < 0){
            ESP_LOGE("run_write_handle", "Error during write in run_tcp: errno %d", errno);
        }

        if(rv != write_handle->nbufs * write_handle->bufs->len){
            /*  Should be called again in order to write everything */
            write_handle->bufs->base += rv;
            write_handle->bufs->len = write_handle->nbufs * write_handle->bufs->len - rv;
            write_handle->nbufs = 1;
            return;
        }

        write_handle->cb(write_handle, write_handle->status);

        rv = uv_remove_handle(write_handle->loop->loopFSM->user_data, handle);
        if(rv != 0){
            ESP_LOGE("run_read_stop_handle", "Error during uv_remove in run_read_stop_handle");
            return;
        }
    }
}

static handle_vtbl_t write_handle_vtbl = {
    .run = run_write_handle
};

int
uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb){
    int rv;

    handle->server->write_cb = cb;

    req->req.loop = handle->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &write_handle_vtbl;

    req->bufs = bufs;
    req->cb = cb;
    req->loop = handle->loop;
    req->nbufs = nbufs;
    req->status = 0;
    req->stream = handle;
    req->type = UV_UNKNOWN_HANDLE;

    rv = uv_insert_handle(handle->loop->loopFSM->user_data, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_write","Error during uv_insert in uv_write");
        return 1;
    }

    return 0;
}