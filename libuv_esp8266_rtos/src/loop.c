#include "uv.h"

// FSM states
enum states {
    IDLE,
    SIGNAL
};

// Checking functions (static int that return either 1 or 0)
// Hace falta locks??
static int
check_all_sig_handlers_run (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->n_active_signal_handlers == p_this->n_signal_handlers_run){
        p_this->n_signal_handlers_run = 0;
        return 1;
    }
    return 0;
}

static int
check_is_closing (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    return p_this->loop_is_closing;
}

// FSM functions (static void)

// call every signal handler
static void
run_signal_handling (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    uv_update_time(p_this);
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
        { SIGNAL, check_all_sig_handlers_run, SIGNAL, run_signal_handling },
        { SIGNAL, check_is_closing, IDLE, NULL},
		{ -1, NULL, -1, NULL},
	};

	return fsm_new (SIGNAL, loopFSM_tt, loop);
}

int
uv_loop_init (uv_loop_t* loop){
    loopFSM_t* newLoopFSM;
    memset(loop,0,sizeof(uv_loop_t));
    loop->loopFSM = fsm_new_loopFSM (newLoopFSM);
    return 0;
}

int
uv_loop_close (uv_loop_t* loop){
    loopFSM_t* this = loop->loopFSM->user_data; // es necesario poner esto?
    this->loop_is_closing = 1;
}

uint32_t
uv_now(const uv_loop_t* loop){
    loopFSM_t* this = loop->loopFSM->user_data;
    return this->time;
}

int
uv_run (uv_loop_t* loop){ // uv_run_mode is not neccesary as only one mode is used in raft
    while(true){
        vTaskDelay(LOOP_RATE_MS/portTICK_RATE_MS);
        fsm_fire(loop->loopFSM);
    }
}

static void
uv_update_time(loopFSM_t* loop){
    loop->time = system_get_time();
}

void
uv_create_task_signal (uv_signal_t* handle){
    // signal_cb_param_t parameters = { handle, handle->signum };
    // xTaskCreate((void*)(handle->signal_cb), "signal", 2048, (void*) &parameters, SIGNAL_TASK_PRIORITY, NULL);
    handle->signal_cb(handle, handle->signum);
}