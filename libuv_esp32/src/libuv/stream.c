#include "uv.h"

/* Run function, vtbl and uv_listen */

/* Listen does not need from reactor */
int
uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb){
    // backlog indicates number of connection to be queued (only backlog = 1 is used in raft)
    int rv;

    ESP_LOGI("listen","");

    rv = listen(stream->socket, backlog);
    if(rv != 0){
        ESP_LOGE("run_listen_handle", "Error durign listen in run_listen_handle: errno %d", errno);
    }

    stream->self.data = stream->data;
    cb(stream, rv);
        
    return 0;
}

/* Run function, vtbl and uv_accept */

/* Run accept tries to accept any possible incoming connection. Then removes handle */
void
run_accept_handle(uv_handle_t* handle){
    int rv;
    uv_accept_t* accept_handle = (uv_accept_t*)handle;

    ESP_LOGI("run_accept_handle", "entering");

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
        struct sockaddr name;
        socklen_t len;
        getpeername(rv,&name,&len);
        char addr[len];
        inet_ntop(AF_INET, &(((struct sockaddr_in*)&name)->sin_addr), addr, len);
        ESP_LOGI("RUN_TCP_ACCEPT", "accepting : %s", addr);
        accept_handle->client->socket = rv;

        rv = uv_remove_handle(accept_handle->loop->loop, handle);
        if(rv != 0){
            ESP_LOGE("run_accept_handle", "Unable to remove handle in run_accept_handle");
            return;
        }

        remove_req_from_stream(accept_handle->server, handle);
    }

    ESP_LOGI("run_accept_handle", "exiting");
}

static handle_vtbl_t accept_handle_vtbl = {
    .run = run_accept_handle
};

int
uv_accept(uv_stream_t* server, uv_stream_t* client){
    int rv;

    ESP_LOGI("accept","");
    
    uv_accept_t* req = malloc(sizeof(uv_accept_t));
    if(!req){
        ESP_LOGE("UV_ACCEPT", "Error during malloc in uv_accept");
        return 1;
    }

    req->req.loop = server->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &accept_handle_vtbl;
    req->req.remove = 0;
    req->req.active = 1;

    req->client = client;
    req->loop = server->loop;
    req->server = server;

    /* Initialize client too */
    client->self.loop = server->loop;
    client->self.type = UV_STREAM;
    client->self.vtbl = NULL;

    client->nreqs = 0;
    client->loop = server->loop;
    client->socket = -1;
    client->type = UV_STREAM;

    add_req_to_stream(server, (uv_handle_t*)req);

    /* Add handle */
    rv = uv_insert_handle(server->loop->loop, (uv_handle_t*)req);
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

    ESP_LOGI("run_read_start_handle", "entering");

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(read_start_handle->stream->socket, &readset);

    if(read_start_handle->alloc_cb){
        uv_buf_t* buf = malloc(sizeof(uv_buf_t));
        buf->len = 4*1024;

        read_start_handle->stream->self.data = read_start_handle->stream->data;
        read_start_handle->buf = buf;
        read_start_handle->alloc_cb((uv_handle_t*) read_start_handle->stream, 4*1024, buf);

        read_start_handle->alloc_cb = NULL; // allocation should only be done once per read_start call
    }

    if(select(read_start_handle->stream->socket + 1, &readset, NULL, NULL, &tv) && !read_start_handle->alloc_cb){
        // CUIDADO, raft aumenta el mismo buf.base y reduce buf.len
        ssize_t nread = read(read_start_handle->stream->socket, read_start_handle->buf->base, read_start_handle->buf->len);
        read_start_handle->buf->base += nread;
        read_start_handle->buf->len -= nread;
        if(nread < 0){
            ESP_LOGE("run_read_start_handle", "Error during read in run_read_start_handle: errno %d", errno);
        }

        read_start_handle->stream->self.data = read_start_handle->stream->data;
        read_start_handle->read_cb(read_start_handle->stream, nread, read_start_handle->buf);
        // DO NOT uv_remove the read start handle (uv_read_stop is the one doing that)
    }

    ESP_LOGI("run_read_start_handle", "exiting");
}

static handle_vtbl_t read_start_handle_vtbl = {
    .run = run_read_start_handle
};

int
uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb){
    int rv;
    
    ESP_LOGI("read_start","");

    uv_read_start_t* req = malloc(sizeof(uv_read_start_t));
    if(!req){
        ESP_LOGE("UV_READ_START", "Error during malloc in uv_read_start");
        return 1;
    }

    req->req.loop = stream->loop;
    req->req.type = UV_READ_START;
    req->req.vtbl = &read_start_handle_vtbl;
    req->req.remove = 0;
    req->req.active = 1;

    req->alloc_cb = alloc_cb;
    req->buf = NULL;
    req->loop = stream->loop;
    req->read_cb = read_cb;
    req->stream = stream;

    add_req_to_stream(stream, (uv_handle_t*)req);

    rv = uv_insert_handle(stream->loop->loop, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("UV_READ_START","Error during uv_insert in uv_read_start");
        return 1;
    }

    return 0;
}

/* Doesnt need from rector */

/* Run read_stop function ceases reading started by read_start. First identifies the correspondent read_start handle and then removes it. */

int
uv_read_stop(uv_stream_t* stream){
    int rv;
    loopFSM_t* loop = stream->loop->loop;

    ESP_LOGI("read_stop","");

    /* Stop the correspoding read_start_request */
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->type == UV_READ_START){
            uv_read_start_t* read_start_handle = (uv_read_start_t*)loop->active_handlers[i];
            if(read_start_handle->stream == stream){
                rv = uv_remove_handle(loop, loop->active_handlers[i]);
                if(rv != 0){
                    ESP_LOGE("run_read_stop_handle", "Error during uv_remove in run_read_stop_handle");
                    return 1;
                }

                remove_req_from_stream(stream, loop->active_handlers[i]);
                return 0;
            } 
        }
    }

    return 0;
}

/* Run function, vtbl and uv_write */

/* Run write function tries to write the whole buffer. When done, write_cb is called. */
void
run_write_handle(uv_handle_t* handle){
    int rv;
    uv_write_t* write_handle = (uv_write_t*)handle;
    int status;

    ESP_LOGI("run_write_handle", "entering");

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set writeset;
    FD_ZERO(&writeset);
    FD_SET(write_handle->stream->socket, &writeset);

    if(select(write_handle->stream->socket + 1, NULL, &writeset, NULL, &tv)){
        rv = write(write_handle->stream->socket, write_handle->bufs->base, write_handle->nbufs * write_handle->bufs->len);
        if(rv < 0){
            ESP_LOGE("run_write_handle", "Error during write in run_tcp: errno %d", errno);
            status = -1;
        }

        if(rv != write_handle->nbufs * write_handle->bufs->len){
            /*  Should be called again in order to write everything */
            write_handle->bufs->base += rv;
            write_handle->bufs->len = write_handle->nbufs * write_handle->bufs->len - rv;
            write_handle->nbufs = 1;
            return;
        }

        status = 0;

        write_handle->req.data = write_handle->data;
        write_handle->cb(write_handle, status);

        for(int i = 0; i < write_handle->nbufs; i++){
            free(write_handle->bufs[i].base);
            free(write_handle->bufs);
        }

        rv = uv_remove_handle(write_handle->loop->loop, handle);
        if(rv != 0){
            ESP_LOGE("run_read_stop_handle", "Error during uv_remove in run_read_stop_handle");
            return;
        }

        remove_req_from_stream(write_handle->stream, handle);
    }

    ESP_LOGI("run_write_handle", "exiting");
}

static handle_vtbl_t write_handle_vtbl = {
    .run = run_write_handle
};

int
uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb){
    int rv;

    ESP_LOGI("write","");

    req->req.loop = handle->loop;
    req->req.type = UV_UNKNOWN_HANDLE;
    req->req.vtbl = &write_handle_vtbl;
    req->req.remove = 0;
    req->req.active = 1;

    /* It is neccesary to copy to mem because it is always called from stack */
    req->bufs = malloc(nbufs*sizeof(uv_buf_t));
    for(int i = 0; i < nbufs; i++){
        req->bufs[i].len = bufs[0].len;
        req->bufs[i].base = malloc(req->bufs[i].len);
        memcpy(req->bufs[i].base, bufs[i].base, req->bufs[i].len);
    }

    req->cb = cb;
    req->loop = handle->loop;
    req->nbufs = nbufs;
    req->stream = handle;
    req->type = UV_UNKNOWN_HANDLE;

    add_req_to_stream(handle, (uv_handle_t*)req);
    
    rv = uv_insert_handle(handle->loop->loop, (uv_handle_t*)req);
    if(rv != 0){
        ESP_LOGE("uv_write","Error during uv_insert in uv_write");
        return 1;
    }

    return 0;
}