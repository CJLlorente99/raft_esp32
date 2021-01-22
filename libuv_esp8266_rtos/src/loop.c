#include "loop.h"

// FSM states
enum states {
    IDLE,
    SIGNAL
};

// Checking functions (static int that return either 1 or 0)
// Hace falta locks?? Los handlers inician task que se ejecutan asincronamente
// podría haber problemas
static int
check_all_sig_handlers_run (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->n_active_signal_handlers == p_this->n_signal_handlers_run){
        return 1;
    }
    return 0;
}

// FSM functions (static void)

// call every signal handler
static void
run_signal_handling (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->n_active_signal_handlers > 0){
        for(int i = 0; i < p_this->n_active_signal_handlers; i++){
            uv_create_task_signal(p_this->active_signal_handlers[i]);
            p_this->n_signal_handlers_run++;
        }
    }
}

// FSM init

fsm_t* fsm_new_loopFSM (loopFSM_t* loop)
{
	static fsm_trans_t loopFSM_tt[] = {
        { IDLE, NULL, SIGNAL, run_signal_handling},
        { SIGNAL, check_all_sig_handlers_run, SIGNAL, run_signal_handling },
		{ -1, NULL, -1, NULL},
	};

	return fsm_new (IDLE, loopFSM_tt, loop);
}

int
uv_loop_init (uv_loop_t* loop){
    loopFSM_t* newLoopFSM;
    memset(loop,0,sizeof(uv_loop_t));
    loop->loopFSM = fsm_new_loopFSM (newLoopFSM);
    return 0;
}