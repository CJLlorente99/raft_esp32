#include "uv.h"
#include "loop.h"

// EL UNICO USO QUE SE HACE DEL SIGNAL EN RAFT ES PARA CAPTAR SEÑALES DE INTERRUPCION
// GENERADAS POR EL USER

// ESTO SE PUEDE TRADUCIR (PARA NUESTRO CASO) EN UNA SEÑAL GENERADA POR UN BOTON. FACILITANDO
// LAS COSAS

int 
uv_signal_init (loopFSM_t* loop, uv_signal_t* handle){
  // Check signal_handler has been initialized
  // if(loop->signal_handlers)
  handle->loop = loop;
    return 0;

  // initialize in case it has not been initialize
  // uv_signal_t* handler_buffer[1];
  // loop->signal_handlers = handler_buffer;
}

int
uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
  // If handler array exists, do some check
  if(handle->loop->active_signal_handlers){
    // Check if handler with same signum
    for(int i = 0; i < handle->loop->n_active_signal_handlers; i++){
      if(handle->loop->active_signal_handlers[i]->signum == signum){
        handle->signal_cb = signal_cb;
        return 0;
      }
    }
  }
  // "Fill" handle

  handle->signal_cb = signal_cb;
  handle->signum = signum;

  // Set signum pin as input
  GPIO_AS_INPUT(signum);

  // If we have achieved to get here, create new handle and add it to signal_handlers 
  uv_signal_t** handlers = handle->loop->active_signal_handlers;
  int i = handle->loop->n_active_signal_handlers; // array index

  if(handle->loop->n_active_signal_handlers == 0){
    *handlers = malloc(sizeof(uv_signal_t));
    memcpy((uv_signal_t*)handlers[0], handle, sizeof(uv_signal_t));
  } else {
    *handlers = realloc(*handlers, sizeof(uv_signal_t));
    memcpy((uv_signal_t*)handlers[i], handle, sizeof(uv_signal_t));
  }

  handle->loop->n_active_signal_handlers++;

  return 0;

}

// Como resumen de lo que hacía en el libuv original. Init abre un pipe para recoger
// las señales deseadas. Start asigna el cb deseado.

// La inicializacion se deberia traducir en una allocacion del array que contiene
// este tipo de handlers en el FSM
// Con start se asigna un callback a este handler
// int uv_signal_init(uv_loop_t* loop, uv_signal_t* handle){
//     int err;

//     err = uv__signal_loop_once_init(loop); // introducir un pipe para canalizar las señal correspondiente
//     if (err) // si este pipe ya esta creado
//         return err;

//     uv__handle_init(loop, (uv_handle_t*) handle, UV_SIGNAL);
//     handle->signum = 0;
//     handle->caught_signals = 0;
//     handle->dispatched_signals = 0;

//     return 0;
// }

// Parece que crea un pipe conectando la señal signal_pipefd[1] con signal_pipefd[0]
// , que parece ser con lo que nosotros tratamos 
// static int uv__signal_loop_once_init(uv_loop_t* loop) {
//   int err;

//   /* Return if already initialized. */
//   if (loop->signal_pipefd[0] != -1)
//     return 0;

//   err = uv__make_pipe(loop->signal_pipefd, UV__F_NONBLOCK); // creacion del pipe
//   if (err)
//     return err;

//   uv__io_init(&loop->signal_io_watcher,
//               uv__signal_event,
//               loop->signal_pipefd[0]); // relaciona un watcher con un cb y un fd
//   uv__io_start(loop, &loop->signal_io_watcher, POLLIN); // introducir el watcher dentro del grupo al que se debe hacer poll para ver si llega algo

//   return 0;
// }

// Asegurarme que solo se va a usar signal start con ultimo argumento = 0
// int uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
//   return uv__signal_start(handle, signal_cb, signum, 0);
// }

// static int uv__signal_start(uv_signal_t* handle,
//                             uv_signal_cb signal_cb,
//                             int signum,
//                             int oneshot) {
//   sigset_t saved_sigmask;
//   int err;
//   uv_signal_t* first_handle;

//   assert(!uv__is_closing(handle));

//   /* If the user supplies signum == 0, then return an error already. If the
//    * signum is otherwise invalid then uv__signal_register will find out
//    * eventually.
//    */
//   if (signum == 0)
//     return UV_EINVAL;

//   /* Short circuit: if the signal watcher is already watching {signum} don't
//    * go through the process of deregistering and registering the handler.
//    * Additionally, this avoids pending signals getting lost in the small
//    * time frame that handle->signum == 0.
//    */
//   if (signum == handle->signum) {
//     handle->signal_cb = signal_cb;
//     return 0;
//   }

//   /* If the signal handler was already active, stop it first. */
//   if (handle->signum != 0) {
//     uv__signal_stop(handle);
//   }

//   uv__signal_block_and_lock(&saved_sigmask);

//   /* If at this point there are no active signal watchers for this signum (in
//    * any of the loops), it's time to try and register a handler for it here.
//    * Also in case there's only one-shot handlers and a regular handler comes in.
//    */
//   first_handle = uv__signal_first_handle(signum);
//   if (first_handle == NULL ||
//       (!oneshot && (first_handle->flags & UV_SIGNAL_ONE_SHOT))) {
//     err = uv__signal_register_handler(signum, oneshot);
//     if (err) {
//       /* Registering the signal handler failed. Must be an invalid signal. */
//       uv__signal_unlock_and_unblock(&saved_sigmask);
//       return err;
//     }
//   }

//   handle->signum = signum;
//   if (oneshot)
//     handle->flags |= UV_SIGNAL_ONE_SHOT;

//   RB_INSERT(uv__signal_tree_s, &uv__signal_tree, handle);

//   uv__signal_unlock_and_unblock(&saved_sigmask);

//   handle->signal_cb = signal_cb;
//   uv__handle_start(handle);

//   return 0;
// }