#include "uv.h"

#define LED_DEBUG_PORT 5

// FSM states
enum states {
    IDLE,
    RUN
};

// Checking functions (static int that return either 1 or 0)
// Hace falta locks??
static int
check_all_handlers_run (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->n_active_handlers == p_this->n_handlers_run){
        p_this->n_handlers_run = 0;
        return 1;
    }
    return 0;
}

static int
check_is_closing (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    return p_this->loop_is_closing;
}

static int
check_is_starting (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    return p_this->loop_is_starting;
}

// FSM functions (static void)

// call every signal handler
static void
run_handlers (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    p_this->loop_is_starting = 0;
    uv_update_time(p_this);
    if(p_this->n_active_handlers > 0){
        for(int i = 0; i < p_this->n_active_handlers; i++){
            uv_update_time(p_this);
            handle_run(p_this->active_handlers[i]);
            p_this->n_handlers_run++;
        }
    }
}

// FSM init

fsm_t* fsm_new_loopFSM (loopFSM_t* loop)
{
	static fsm_trans_t loopFSM_tt[] = {
        { RUN, check_all_handlers_run, RUN, run_handlers },
        { RUN, check_is_closing, IDLE, NULL},
        { IDLE, check_is_starting, RUN, run_handlers},
		{ -1, NULL, -1, NULL},
	};

	return fsm_new (IDLE, loopFSM_tt, loop);
}

int
uv_loop_init (uv_loop_t* loop){
    loopFSM_t* newLoopFSM = malloc(sizeof(loopFSM_t));

    if(!newLoopFSM){
        ESP_LOGE("LOOP_INIT", "Error during malloc in loop init");
        return 1;
    }

    struct timeval tv;
    if(gettimeofday(&tv, NULL)){
        ESP_LOGE("LOOP_INIT", "Error during gettimeofday in loop init");
        return 1;
    }

    newLoopFSM->active_handlers = NULL;
    newLoopFSM->n_active_handlers = 0;
    newLoopFSM->n_handlers_run = 0;
    newLoopFSM->loop_is_closing = 0;
    newLoopFSM->loop_is_starting = 1;
    newLoopFSM->time = tv.tv_usec/1000;

    loop->loopFSM = fsm_new_loopFSM (newLoopFSM);

    if(!loop->loopFSM){
        ESP_LOGE("LOOP_INIT", "Error during fsm creation");
        return 1;
    }
    return 0;
}

int
uv_loop_close (uv_loop_t* loop){
    loopFSM_t* this = loop->loopFSM->user_data; // es necesario poner esto?
    this->loop_is_closing = 1;
    return 0;
}

uint32_t
uv_now(const uv_loop_t* loop){
    loopFSM_t* this = loop->loopFSM->user_data;
    return this->time;
}

int
uv_run (uv_loop_t* loop){ // uv_run_mode is not neccesary as only one mode is used in raft
    portTickType xLastTime = xTaskGetTickCount();
    const portTickType xFrequency = LOOP_RATE_MS/portTICK_RATE_MS;
    ESP_LOGI("UV_RUN", "Entering uv_run loop");
    while(true){
        fsm_fire(loop->loopFSM);
        vTaskDelayUntil(&xLastTime, xFrequency);
    }
    return 1;
}

void
uv_update_time(loopFSM_t* loop){
    struct timeval tv;
    if(gettimeofday(&tv, NULL)){
        ESP_LOGE("UV_UPDATE_TIME", "Error during gettimeofday in uv_update_time");
        return;
    }
    loop->time = tv.tv_usec/1000;
}

void
handle_run(uv_handle_t* handle){
    // AQUI HAY UN ERROR!
    if (!(handle->vtbl->run)){
        ESP_LOGE("HANDLE_RUN", "Error when calling run method in handle_run");
        return;
    }
    handle->vtbl->run(handle);
}