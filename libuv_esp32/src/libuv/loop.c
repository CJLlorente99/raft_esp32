#include "uv.h"

#define LED_DEBUG_PORT 5

// FSM states
enum states {
    IDLE,
    RUN,
    REQUESTS
};

// Checking functions (static int that return either 1 or 0)
static int
check_all_handlers_run (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->last_n_active_handlers == p_this->n_handlers_run){
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

/* Call every handler */
static void
run_handlers (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    p_this->loop_is_starting = 0;
    uv_update_time(p_this);
    p_this->last_n_active_handlers = p_this->n_active_handlers;
    for(int i = 0; i < p_this->last_n_active_handlers; i++){
        uv_update_time(p_this);
        handle_run(p_this->active_handlers[i]);
        p_this->n_handlers_run++;
    }
}

// FSM init
fsm_t* fsm_new_loopFSM (loopFSM_t* loop)
{
	static fsm_trans_t loopFSM_tt[] = {
        { RUN, check_all_handlers_run, RUN, run_handlers },
        { RUN, check_is_closing, IDLE, NULL},
        { REQUESTS, check_is_closing, IDLE, NULL},
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

    uint32_t begin = pdTICKS_TO_MS(xTaskGetTickCount());
    if(!begin){
        ESP_LOGE("LOOP_INIT", "Error during gettimeofday in loop init");
        return 1;
    }

    newLoopFSM->n_active_handlers = 0;
    newLoopFSM->n_handlers_run = 0;
    newLoopFSM->loop_is_closing = 0;
    newLoopFSM->loop_is_starting = 1;
    newLoopFSM->time = begin;
    newLoopFSM->signal_isr_activated = 0;

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
uv_run (uv_loop_t* loop, uv_run_mode mode){ // uv_run_mode is not neccesary as only one mode is used in raft
    portTickType xLastTime = xTaskGetTickCount();
    const portTickType xFrequency = LOOP_RATE_MS/portTICK_RATE_MS;
    ESP_LOGI("UV_RUN", "Entering uv_run loop");
    while(true){
        fsm_fire(loop->loopFSM);
        // añadir sleep mode (CUIDADO CON LAS WIFI Y LAS CONEXIONES TCP)
        vTaskDelayUntil(&xLastTime, xFrequency);
    }
    return 1;
}

void
uv_update_time(loopFSM_t* loop){
    uint32_t act_time = pdTICKS_TO_MS(xTaskGetTickCount());
    if(!act_time){
        ESP_LOGE("UV_UPDATE_TIME", "Error during gettimeofday in uv_update_time");
        return;
    }
    loop->time = act_time;
}

void
handle_run(uv_handle_t* handle){
    if (!(handle->vtbl->run)){
        ESP_LOGE("HANDLE_RUN", "Error when calling run method in handle_run");
        return;
    }
    handle->vtbl->run(handle);
}

void
uv_close(uv_handle_t* handle, uv_close_cb close_cb){
    int rv = 0;
    rv = uv_remove_handle(handle->loop->loopFSM, handle);
    if(rv != 0){
        ESP_LOGE("UV_CLOSE", "Error when calling uv_remove_handle in uv_close");
    }
}