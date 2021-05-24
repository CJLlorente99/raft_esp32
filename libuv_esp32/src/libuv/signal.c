#include "uv.h"

static void IRAM_ATTR signal_isr(void* args);

/* Run function, vtbl and uv_signal */
static void
run_signal(uv_handle_t* handle){
    ESP_LOGI("run_signal","entering");
    uv_signal_t* signal = (uv_signal_t*) handle;
    if(signal->intr_bit){
        handle->data = signal->data;
        signal->signal_cb(signal, signal->signum);
        signal->intr_bit = 0;
    }

    ESP_LOGI("run_signal","exiting");
}

static handle_vtbl_t signal_vtbl = {
    .run = run_signal
};

int 
uv_signal_init (uv_loop_t* loop, uv_signal_t* handle){

    handle->self.loop = loop;
    handle->self.type = UV_SIGNAL;
    handle->self.vtbl = &signal_vtbl;
    handle->self.remove = 0;

    handle->data = NULL;
    handle->intr_bit = 0;
    handle->loop = loop;
    handle->signal_cb = NULL;
    handle->signum = -1;
    handle->type = UV_SIGNAL;

    return 0;
}

int
uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
    loopFSM_t* loop = handle->loop->loop;
    int rv = 0;
    esp_err_t err;

    /* Find for handler with same signum. If exists, remove because it will be updated */
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->type == UV_SIGNAL){
            uv_signal_t* signal = (uv_signal_t*)loop->active_handlers[i];
            if(signal->signum == signum){
                rv = uv_remove_handle(loop, (uv_handle_t*)signal);
                if(rv != 0){
                    ESP_LOGE("UV_SIGNAL_STOP", "Error when calling uv_remove_handle in uv_signal_stop");
                    return 1;
                }
            }
        }
    }

    handle->signal_cb = signal_cb;
    handle->signum = signum;
    handle->self.active = 1;

    /* Init interrupt for given signum */

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
        loop->signal_isr_activated = 1;
    }
    
    err = gpio_isr_handler_add(signum, signal_isr, (void*) handle);
    if (err != 0){
        ESP_LOGE("UV_SIGNAL_START", "Error during gpio_isr_handler_add in uv_signal_start: err %x", err);
        return 1;
    }

    rv = uv_insert_handle(loop, (uv_handle_t*)handle);
    if (rv != 0){
        ESP_LOGE("UV_SIGNAL_START", "Error when calling uv_insert_handle from uv_signal_start");
        return 1;
    }
    
    return 0;
}

int
uv_signal_stop(uv_signal_t* handle){
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

    handle->self.active = 0;

    return 0;
}

static void IRAM_ATTR
signal_isr(void* args){
    uv_signal_t* signal = (uv_signal_t*) args;
    signal->intr_bit = 1;
}