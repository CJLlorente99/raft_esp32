#include "uv.h"

// EL UNICO USO QUE SE HACE DEL SIGNAL EN RAFT ES PARA CAPTAR SEÑALES DE INTERRUPCION
// GENERADAS POR EL USER

// virtual table for signal handlers
static handle_vtbl_t signal_vtbl = {
    .run = run_signal
};


int 
uv_signal_init (uv_loop_t* loop, uv_signal_t* handle){
    handle->self->loop = loop;
    handle->self->vtbl = &signal_vtbl;
    return 0;
}

int
uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;
    // If handler array exists, do some check
    if(loop->active_handlers){
        // Check if handler with same signum
        uv_signal_t** handlers = loop->active_handlers;
        for(int i = 0; i < loop->n_active_handlers; i++){
            if(handlers[i]->signum){ // cuidado aqui, puede haber problemas?
                if(handlers[i]->signum == signum){
                    handle->signal_cb = signal_cb;
                    return 0;
                }
            }
        }
    }
    // "Fill" handle

    handle->signal_cb = signal_cb;
    handle->signum = signum;

    // Init interrupt for given signum

    GPIO_AS_INPUT(signum);
    gpio_pin_intr_state_set(signum, GPIO_PIN_INTR_HILEVEL);
    gpio_intr_handler_register(&signal_isr, loop); // maybe this should be called only once.
    // to see which gpio caused the interruption -> gpio_input_get (returns bitmask of GPIO input pins);

    // If we have achieved to get here, create new handle and add it to signal_handlers 
    uv_handle_t** handlers = loop->active_handlers;
    int i = loop->n_active_handlers; // array index

    if(loop->n_active_handlers == 0){
        *handlers = malloc(sizeof(uv_signal_t));
        memcpy((uv_handle_t*)handlers[0], handle->self, sizeof(uv_handle_t));
    } else {
        *handlers = realloc(*handlers, sizeof(uv_handle_t[i]));
        memcpy((uv_handle_t*)handlers[i], handle->self, sizeof(uv_handle_t));
    }

    loop->n_active_handlers++;

    return 0;

}

int
uv_signal_stop(uv_signal_t* handle){
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;

    // Allocate memory for new array of handlers
    int new_n_active_handlers = loop->n_active_handlers--;
    uv_handle_t** new_handlers = malloc(sizeof(uv_handle_t[new_n_active_handlers]));

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->signum){
            if(loop->active_handlers[i]->handle_signal != handle){
                memcpy((uv_handle_t*)new_handlers[j++], loop->active_handlers[i], sizeof(uv_handle_t));
            }
        }
    }

    // Exchange in loop structure
    loop->active_handlers = new_handlers;

    return 0;
}

void
signal_isr(loopFSM_t* loop){
    uint32 bitmask = gpio_input_get();

    for (int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i]->){
            if ((bitmask >> loop->active_handlers[i]->handle_signal->signum) & 1){
                loop->active_handlers[i]->handle_signal->intr_bit = 1;
            }
            // is this option also possible?
            // loop->active_handlers[i]->handle_signal->intr_bit = ((bitmask >> loop->active_handlers[i]->handle_signal->signum) & 1);
            
        }
    }
    
}

// run implementation for signals
void
run_signal(uv_signal_t* signal){
    if(signal->intr_bit){
        signal->signal_cb(signal, signal->signum);
        signal->intr_bit = 0;
    }
    else{
        signal->intr_bit = 0;
    }
}