#ifndef UV_H
#define UV_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fsm.h"
#include "gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_common.h"

// Some global constants

#define SIGNAL_TASK_PRIORITY 4
#define LOOP_RATE_MS 100

// Declaration

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_signal_s uv_signal_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_buf_s uv_buf_t;

typedef struct signal_cb_param_s signal_cb_param_t;

typedef struct loopFSM_s loopFSM_t;

// Callbacks declaration and definition

typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);
typedef void (*uv_timer_cb)(uv_timer_t* handle);
typedef void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
typedef void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
typedef void (*uv_connection_cb)(uv_stream_t* server, int status);
typedef void (*uv_close_cb)(uv_handle_t* handle);


// Structs for parameters to create task

struct signal_cb_param_s {
  uv_signal_t* handle;
  int signum;
};


// Types definition

struct uv_tcp_s {
  uv_loop_t* loop;
  unsigned int flags;
  uv_read_cb* read_cb;
  uv_alloc_cb* alloc_cb;
  uv_connection_cb* connection_cb;
  uv_close_cb* close_cb;
};

struct uv_stream_s {
  uv_loop_t* loop;
  unsigned int flags;
  uv_read_cb* read_cb;
  uv_alloc_cb* alloc_cb;
  uv_connection_cb* connection_cb;
  uv_close_cb* close_cb;
};

struct uv_buf_s {
  char* base;
  size_t len;
};

struct uv_timer_s {
  uv_loop_t* loop;
  os_timer_t* timer;
  uv_timer_cb timer_cb;
  unsigned int timeout;
  unsigned int repeat : 1;
};

struct uv_signal_s {
  uv_loop_t* loop;
  uv_signal_cb signal_cb;
  int signum; // indicates pin
};

struct uv_loop_s {
  fsm_t* loopFSM;
};

//Fsm needed data
struct loopFSM_s
{
  uint32_t time;

  unsigned int loop_is_closing : 1;

  uv_signal_t** active_signal_handlers; // asi, al añadir nuevos handler no hace falta volver a crear el fsm_t. con este puntero y el numero de handlers itero sobre todos
  unsigned int n_active_signal_handlers; // number of signal handlers
  unsigned int n_signal_handlers_run; // number of signal handlers that have been run

  uv_stream_t** active_stream_handlers;
  unsigned int n_active_stream_handlers;
  unsigned int n_stream_handlers_run;

};

// Some function prototypes

void uv_update_time (loopFSM_t* loop);

// Timer function protypes

int uv_timer_init(uv_loop_t* loop_s, uv_timer_t* handle);
int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int uv_timer_stop(uv_timer_t* handle);

// Signal function prototypes

int uv_signal_init (uv_loop_t* loop, uv_signal_t* handle);
int uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum);
int uv_signal_stop(uv_signal_t* handle);

// Core function protypes

void uv_create_task_signal (uv_signal_t* handle);

// Loop function prototypes

int uv_loop_init (uv_loop_t* loop);
int uv_loop_close (uv_loop_t* loop);
uint32_t uv_now(const uv_loop_t* loop);
int uv_run (uv_loop_t* loop);

#endif /* UV_H */
