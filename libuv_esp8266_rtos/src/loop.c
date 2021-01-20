#include "loop.h"

// Variables
static fsm_t loopFSM;

// FSM states
enum states {
    IDLE,
    SIGNAL
};

// Checking functions (static int that return either 1 or 0)
static int
check_all_sig_handlers_run (fsm_t* this){
    
}

// FSM functions (static void)

// call every signal handler
static void
run_signal_handling (fsm_t* this){
    
}

// FSM init

fsm_t* fsm_new_loopFSM ()
{
	static fsm_trans_t loopFSM_tt[] = {
        { IDLE, NULL, SIGNAL, run_signal_handling},
        // { SIGNAL, CHECK_ALL_SIG_HANDLERS_RUN, NEXT_STATE, run_next_state_handling },
		{ -1, NULL, -1, NULL},
	};

	return fsm_new (IDLE, loopFSM_tt, &loopFSM);
}