#include "uv.h"

// EL UNICO USO QUE SE HACE DEL SIGNAL EN RAFT ES PARA CAPTAR SEÑALES DE INTERRUPCION
// GENERADAS POR EL USER

void run_signal(uv_handle_t* handle);

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

    // TODO
    // comprobar si tiene el mismo signum. Si lo tienen, encontrar el handle y cambiar el signal_cb

    // "Fill" handle

    handle->signal_cb = signal_cb;
    handle->signum = signum;

    // Init interrupt for given signum

    GPIO_AS_INPUT(signum);
    gpio_pin_intr_state_set(signum, GPIO_PIN_INTR_POSEDGE);
    gpio_intr_handler_register(&signal_isr, loop); // maybe this should be called only once.

    insert_handle(loop, (uv_handle_t*)handle);

    return 0;

}

int
uv_signal_stop(uv_signal_t* handle){
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;

    remove_handle(loop, (uv_handle_t*)handle);

    return 0;
}

void
signal_isr(loopFSM_t* loop){
    uint32 bitmask = gpio_input_get();

    // TODO
    // comprobar cual es el handler que ha sido activado y activar el intr_bit
    for (int i = 0; i < loop->n_active_handlers; i++){
        uv_signal_t* signal = (uv_signal_t*)loop->active_handlers[i];
        if(signal->signum){ // what i am trying to do with this is check if reference actually exists (it is a uv_signal_t indeed)
            if ((bitmask >> signal->signum) & 1){
                signal->intr_bit = 1;
            }
            // is this option also possible?
            // loop->active_handlers[i]->handle_signal->intr_bit = ((bitmask >> loop->active_handlers[i]->handle_signal->signum) & 1);
        }
    }
    
}

// run implementation for signals
void
run_signal(uv_handle_t* handle){
    uv_signal_t* signal = (uv_signal_t*) handle;
    if(signal->intr_bit){
        signal->signal_cb(signal, signal->signum);
        signal->intr_bit = 0;
    }
    else{
        signal->intr_bit = 0;
    }
}