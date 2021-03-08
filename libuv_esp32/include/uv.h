#ifndef UV_H
#define UV_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>

#include "fsm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"
#include "ff.h"
#include "esp_log.h"

/// Some global constants

#define SIGNAL_TASK_PRIORITY 4
#define LOOP_RATE_MS 100

/// Declaration

typedef struct handle_vtbl_s handle_vtbl_t;
typedef struct request_vtbl_s request_vtbl_t;

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_signal_s uv_signal_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_tcp_s uv_stream_t;
typedef struct uv_buf_s uv_buf_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_write_s uv_write_t;
typedef struct uv_connect_s uv_connect_t;
typedef struct uv_listen_s uv_listen_t;
typedef struct uv_accept_s uv_accept_t;
typedef struct uv_read_start_s uv_read_start_t;
typedef struct uv_read_stop_s uv_read_stop_t;
typedef struct uv_poll_s uv_poll_t;
typedef struct uv_fs_s uv_fs_t;
typedef struct uv_file_s uv_file;
typedef struct uv_dirent_s uv_dirent_t;
typedef struct uv_request_s uv_request_t;

typedef struct loopFSM_s loopFSM_t;

// For signal purposes
typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);

// For timer purposes
typedef void (*uv_timer_cb)(uv_timer_t* handle);

// For stream/tcp purposes
typedef void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
typedef void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
typedef void (*uv_connection_cb)(uv_stream_t* server, int status);
typedef void (*uv_close_cb)(uv_handle_t* handle);
typedef void (*uv_write_cb)(uv_write_t* req, int status);
typedef void (*uv_connect_cb)(uv_connect_t* req, int status);

// For check purposes
typedef void (*uv_check_cb)(uv_check_t* handle);

// For poll purposes
typedef void (*uv_poll_cb)(uv_poll_t* handle, int status, int events);

// For fs puposes
typedef void (*uv_fs_cb)(uv_fs_t* req);

// handle "class"
struct uv_handle_s {
    handle_vtbl_t* vtbl;
    uv_loop_t* loop;
};

// virtual table for every handle
struct handle_vtbl_s {
    void (*run)(uv_handle_t* handle);
};

// polymorphic method
void handle_run(uv_handle_t* handle);

// request "class"
struct uv_request_s {
    request_vtbl_t* vtbl;
    uv_loop_t* loop;
};

// virtual table for every request
struct request_vtbl_s {
    void (*run)(uv_request_t* handle);
};

// polymorphic method
void request_run(uv_request_t* handle);

/// Types definition

struct uv_write_s {
    uv_request_t req;
    uv_write_cb cb;
    int status;
    uv_buf_t* bufs;
    int nbufs;
};

struct uv_connect_s {
    uv_request_t req;
    struct sockaddr* dest_sockaddr;
    uv_connect_cb cb;
    int status;
};

struct uv_listen_s {
    uv_request_t req;
    uv_stream_t* stream;
    uv_connection_cb cb;
    int status;
};

struct uv_accept_s {
    uv_request_t req;
    uv_stream_t* server;
    uv_stream_t* client;
};

struct uv_read_start_s {
    uv_request_t req;
    uv_stream_t* stream;
    uv_alloc_cb alloc_cb;
    uv_read_cb read_cb;
    uv_buf_t* buf;
    ssize_t nread;
    int is_alloc : 1;
};

struct uv_read_stop_s {
    uv_request_t req;
};

struct uv_fs_s {
    uv_request_t req;
    // request
};

struct uv_file_s {
    // representation of a file
};

struct uv_dirent_s {

};

struct uv_poll_s {
    uv_handle_t* self;
    uv_poll_cb cb;
};


struct uv_check_s {
    uv_handle_t self;
    uv_check_cb cb;
};

struct uv_tcp_s {
    uv_handle_t self;
    uint64_t flags;
    uv_read_cb read_cb;
    uv_alloc_cb alloc_cb;
    uv_connection_cb connection_cb;
    uv_close_cb close_cb;
    uv_connect_cb connect_cb;
    uv_write_cb write_cb;
    int socket;
    struct sockaddr* src_sockaddr;
    fd_set* readset;
    fd_set* writeset;
    fd_set* errorset;
    
    int bind : 1;

    uv_connect_t** connect_requests;
    int n_connect_requests;

    uv_accept_t** accept_requests;
    int n_accept_requests;

    uv_listen_t** listen_requests;
    int n_listen_requests;

    uv_read_start_t** read_start_requests;
    int n_read_start_requests;

    uv_read_stop_t** read_stop_requests;
    int n_read_stop_requests;

    uv_write_t** write_requests;
    int n_write_requests;
};

struct uv_buf_s {
    char* base;
    size_t len;
};

struct uv_timer_s {
    uv_handle_t self;
    uv_timer_cb timer_cb;
    uint32_t timeout;
    uint32_t repeat;
};

struct uv_signal_s {
    uv_handle_t self;
    uv_signal_cb signal_cb;
    int signum; // indicates pin
    uint64_t intr_bit : 1;
};

struct uv_loop_s {
    fsm_t* loopFSM;
};

//Fsm needed data
struct loopFSM_s
{
    uint32_t time;

    int loop_is_closing : 1;
    int loop_is_starting : 1;

    uv_handle_t** active_handlers; // asi, al añadir nuevos handler no hace falta volver a crear el fsm_t. con este puntero y el numero de handlers itero sobre todos
    int n_active_handlers; // number of signal handlers
    int n_handlers_run; // number of signal handlers that have been run

    uv_request_t** active_requests;
    int n_active_requests;
    int all_requests_run : 1;
};

// Some function prototypes

void uv_update_time (loopFSM_t* loop);

// Timer function protypes

int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle);
int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int uv_timer_stop(uv_timer_t* handle);
int uv_timer_again(uv_timer_t* handle);

// Signal function prototypes

int uv_signal_init(uv_loop_t* loop, uv_signal_t* handle);
int uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum);
int uv_signal_stop(uv_signal_t* handle);

// Check function prototypes

int uv_check_init(uv_loop_t* loop, uv_check_t* check);
int uv_check_start(uv_check_t* handle, uv_check_cb cb);
int uv_check_close(uv_check_t* handle);

// Loop function prototypes

int uv_loop_init (uv_loop_t* loop);
int uv_loop_close (uv_loop_t* loop);
int uv_run (uv_loop_t* loop);

// Core function prototypes
int insert(void** pointer, int* active, size_t size, void* handle);
int remove(void** pointer, int* active, size_t size, void* handle);

// Request run implementations prototypes
void run_connect_req(uv_request_t* req);
void run_listen_req(uv_request_t* req);
void run_accept_req(uv_request_t* req);
void run_read_start_req(uv_request_t* req);
void run_read_stop_req(uv_request_t* req);
void run_write_req(uv_request_t* req);

#endif /* UV_H */
