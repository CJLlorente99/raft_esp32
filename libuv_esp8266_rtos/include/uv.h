#ifndef UV_H
#define UV_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fsm.h"

// Some global constants

#define SIGNAL_TASK_PRIORITY 4

// Callbacks declaration and definition

typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);

// Structs for parameters to create task

typedef struct signal_cb_param {
  uv_signal_t* handle;
  int signum;
} signal_cb_param;


// Types definition

typedef struct uv_signal_t {
  loopFSM_t* loop;
  uv_signal_cb signal_cb;
  int signum;
} uv_signal_t;

typedef struct uv_loop_t {
  fsm_t* loopFSM;
} uv_loop_t;
#endif /* UV_H */
