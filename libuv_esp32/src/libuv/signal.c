#include "uv.h"

#define LED_DEBUG_PORT 5

static void IRAM_ATTR signal_isr(void* args);

// EL UNICO USO QUE SE HACE DEL SIGNAL EN RAFT ES PARA CAPTAR SEÑALES DE INTERRUPCION
// GENERADAS POR EL USER

// run implementation for signals
static void
run_signal(uv_handle_t* handle){
    uv_signal_t* signal = (uv_signal_t*) handle;
    if(signal->intr_bit){
        signal->signal_cb(signal, signal->signum);
        signal->intr_bit = 0;
    }
}

// virtual table for signal handlers
static handle_vtbl_t signal_vtbl = {
    .run = run_signal
};

int 
uv_signal_init (uv_loop_t* loop, uv_signal_t* handle){

    handle->loop = loop;
    handle->self.vtbl = &signal_vtbl;

    handle->intr_bit = 0;
    handle->signal_cb = NULL;
    handle->signum = 0;
    handle->type = UV_SIGNAL;

    return 0;
}

int
uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
    loopFSM_t* loop = handle->loop->loopFSM->user_data;
    int rv = 0;
    esp_err_t err;
    // TODO
    // comprobar si tiene el mismo signum. Si lo tienen, encontrar el handle y cambiar el signal_cb

    // "Fill" handle

    handle->signal_cb = signal_cb;
    handle->signum = signum;

    // Init interrupt for given signum

    ESP_LOGI("UV_SIGNAL_START", "Setting interrupt for signum %d", signum);

    err = gpio_intr_enable(signum);
    if(err != 0){
        ESP_LOGE("UV_SIGNAL_START", "Error during gpio_intr_enable in uv_signal_start: err %d", err);
        return 1;
    }

    err = gpio_set_intr_type(signum, GPIO_INTR_POSEDGE); // can also be done through gpio_config
    if(err != 0){
        ESP_LOGE("UV_SIGNAL_START", "Error during gpio_set_intr_type in uv_signal_start: err %d", err);
        return 1;
    }
    
    if(!loop->signal_isr_activated){
        err = gpio_install_isr_service(0);
        if(err != 0){
            ESP_LOGE("UV_SIGNAL_START", "Error during gpio_isr_register in uv_signal_start: err %x", err);
            return 1;
        }
    }
    
    err = gpio_isr_handler_add(signum, signal_isr, (void*) handle);
    if (err != 0){
        ESP_LOGE("UV_SIGNAL_START", "Error during gpio_isr_handler_add in uv_signal_start: err %x", err);
        return 1;
    }

    loop->signal_isr_activated = 1;

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if (rv != 0){
        ESP_LOGE("UV_SIGNAL_START", "Error when calling uv_insert_handle from uv_signal_start");
        return 1;
    }
    
    return 0;
}

int
uv_signal_stop(uv_signal_t* handle){
    loopFSM_t* loop = handle->loop->loopFSM->user_data;
    int rv;
    esp_err_t err;

    err = gpio_intr_disable(handle->signum);
    if (err != 0){
        ESP_LOGE("UV_SIGNAL_STOP", "Error when calling gpio_intr_disable in uv_signal_stop: err %x", err);
        return 1;
    }

    err = gpio_isr_handler_remove(handle->signum);
    if(err != 0){
        ESP_LOGE("UV_SIGNAL_STOP", "Error when calling gpio_isr_handler_remove in uv_signal_stop: err %x", err);
        return 1;
    }

    rv = uv_remove_handle(loop, (uv_handle_t*)handle);
    if(rv != 0){
        ESP_LOGE("UV_SIGNAL_STOP", "Error when calling uv_remove_handle in uv_signal_stop");
        return 1;
    }

    return 0;
}

static void IRAM_ATTR
signal_isr(void* args){
    uv_signal_t* signal = (uv_signal_t*) args;
    signal->intr_bit = 1;
}