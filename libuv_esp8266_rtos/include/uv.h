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
#include "esp_common.h"

// Some global constants

#define SIGNAL_TASK_PRIORITY 4
#define LOOP_RATE_MS 100

// Declaration

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_signal_s uv_signal_t;

typedef struct signal_cb_param_s signal_cb_param_t;

typedef struct loopFSM_s loopFSM_t;

// Callbacks declaration and definition

typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);

// Structs for parameters to create task

struct signal_cb_param_s {
  uv_signal_t* handle;
  int signum;
};


// Types definition

struct uv_signal_s {
  loopFSM_t* loop;
  uv_signal_cb signal_cb;
  int signum; // indicates pin
  unsigned int is_active : 1;
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
};

// Some function prototypes

void uv_update_time (loopFSM_t* loop);

#endif /* UV_H */
