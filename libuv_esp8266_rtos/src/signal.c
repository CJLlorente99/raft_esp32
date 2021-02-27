#include "uv.h"

#define LED_DEBUG_PORT 5

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
    else{
        signal->intr_bit = 0;
    }
    printf("Signal with signum %d run\n", signal->signum);
}

// virtual table for signal handlers
static handle_vtbl_t signal_vtbl = {
    .run = run_signal
};

int 
uv_signal_init (uv_loop_t* loop, uv_signal_t* handle){
    handle->self = malloc(sizeof(uv_handle_t));

    if(!handle->self){
        printf("Fallo en el malloc de signal_init \n");
        return 1;
    }

    handle->self->loop = loop;
    handle->self->vtbl = &signal_vtbl;

    // printf("Puntero funcion run %d\n",handle->self->vtbl);
    // printf("Puntero funcion run %d\n",(*(uv_handle_t**)handle)->vtbl);

    handle->intr_bit = 0;
    handle->signal_cb = NULL;
    handle->signum = 0;
    return 0;
}

int
uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;
    int rv = 0;
    // TODO
    // comprobar si tiene el mismo signum. Si lo tienen, encontrar el handle y cambiar el signal_cb

    // "Fill" handle

    handle->signal_cb = signal_cb;
    handle->signum = signum;

    // Init interrupt for given signum

    printf("Setting interrupt for signum %d\n", signum);

    portENTER_CRITICAL();
    gpio_pin_intr_state_set((uint32_t)signum, GPIO_PIN_INTR_NEGEDGE);
    gpio_intr_handler_register(signal_isr, &loop); // maybe this should be called only once.
    portEXIT_CRITICAL();

    // Hay un fallo aqui
    rv = insert_handle(loop, (uv_handle_t*)handle);
    if (rv != 0){
        printf("Fallo en el insert handle de signal start");
        return 1;
    }
    

    return 0;

}

int
uv_signal_stop(uv_signal_t* handle){
    loopFSM_t* loop = handle->self->loop->loopFSM->user_data;

    remove_handle(loop, (uv_handle_t*)handle);

    return 0;
}

static void
signal_isr(loopFSM_t* loop){
    printf("Signal interrupt has taken place");
    GPIO_OUTPUT_SET(LED_DEBUG_PORT,1);
    // comprobar cual es el handler que ha sido activado y activar el intr_bit
    for (int i = 0; i < loop->n_active_handlers; i++){
        uv_signal_t* signal = (uv_signal_t*)loop->active_handlers[i];
        if(signal->signum){ // what i am trying to do with this is check if reference actually exists (it is a uv_signal_t indeed)
            if (GPIO_INPUT_GET(signal->signum)){
                signal->intr_bit = 1;
            }
            // is this option also possible?
            // loop->active_handlers[i]->handle_signal->intr_bit = ((bitmask >> loop->active_handlers[i]->handle_signal->signum) & 1);
        }
    }
    
}