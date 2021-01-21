#include "uv.h"

#ifndef _LOOP_FSM_
#define _LOOP_FSM_

// Useful constants

// Flags
typedef struct flags_t
{
    // unsigned int all_signal_handlers : 1; // could be done just with n_signal_handlers run (in loopFSM_t)
} flags_t;


//Fsm needed data
typedef struct loopFSM_t
{
    uv_signal_t** active_signal_handlers; // asi, al añadir nuevos handler no hace falta volver a crear el fsm_t. con este puntero y el numero de handlers itero sobre todos
    unsigned int n_active_signal_handlers; // number of signal handlers
    unsigned int n_signal_handlers_run; // number of signal handlers that have been run
} loopFSM_t;

#endif /* _TREE_FSM_ */