#include "uv.h"

#define LED_DEBUG_PORT 5

// TODO
// Creo que esto no esta bien. Debería ser void*** pointer?

int
uv_insert_handle(loopFSM_t* loop, uv_handle_t* handle){
    int i = loop->n_active_handlers + 1; // array index

    if(i == 1){
        loop->active_handlers = malloc(sizeof(uv_handle_t*));
        if(!loop->active_handlers){
            ESP_LOGE("UV_INSERT_HANDLE", "Error pointer is NULL after malloc in uv_insert_handle");
            return 1;
        }
            
        memcpy(loop->active_handlers, &handle, sizeof(uv_handle_t*));
    } else{
        loop->active_handlers = realloc(loop->active_handlers, i * sizeof(uv_handle_t*));
        if(!loop->active_handlers){
            ESP_LOGE("UV_INSERT_HANDLE", "Error pointer is NULL after malloc in uv_insert_handle");
            return 1;
        }
        memcpy(&(loop->active_handlers[i-1]), &handle, sizeof(uv_handle_t*));
    }

    loop->n_active_handlers = i;

    return 0;
}

int
uv_remove_handle(loopFSM_t* loop, uv_handle_t* handle){
    // Allocate memory for new array of handlers
    int new_n_active = loop->n_active_handlers - 1;
    uv_handle_t** new_pointer = malloc(new_n_active * sizeof(uv_handle_t*));

    if(!new_pointer){
        ESP_LOGE("UV_REMOVE_HANDLE", "Error new_pointer is NULL after malloc in uv_remove_handles");
        return 1;
    }

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_handlers; i++){
        if(loop->active_handlers[i] != handle){
            memcpy(&(new_pointer[j++]), &(loop->active_handlers[i]), sizeof(uv_handle_t*));
        }
    }

    loop->n_active_handlers = new_n_active;
    loop->active_handlers = new_pointer;

    return 0;
}

int
uv_insert_request(loopFSM_t* loop, uv_request_t* req){
    int i = (loop->n_active_requests)++; // array index

    if(i == 1){
        loop->active_requests = malloc(sizeof(uv_request_t*));
        if(!loop->active_requests){
            ESP_LOGE("UV_INSERT_REQUESTS", "Error pointer is NULL after malloc in uv_insert_requests");
            return 1;
        }
            
        memcpy(loop->active_requests, &req, sizeof(uv_request_t*));
    } else{
        loop->active_requests = realloc(loop->active_requests, i * sizeof(uv_request_t*));
        if(!loop->active_requests){
            ESP_LOGE("UV_INSERT_REQUESTS", "Error pointer is NULL after malloc in uv_insert_requests");
            return 1;
        }
        memcpy(&(loop->active_requests[i-1]), &req, sizeof(uv_request_t*));
    }

    loop->n_active_requests = i;

    return 0;
}

int
uv_remove_request(loopFSM_t* loop, uv_request_t* req){
    // Allocate memory for new array of handlers
    int new_n_active = loop->n_active_requests - 1;
    uv_request_t** new_pointer = malloc(new_n_active * sizeof(uv_request_t*));

    if(!new_pointer){
        ESP_LOGE("UV_REMOVE_REQUESTS", "Error new_pointer is NULL after malloc in uv_remove_requests");
        return 1;
    }

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < loop->n_active_requests; i++){
        if(loop->active_requests[i] != req){
            memcpy(&(new_pointer[j++]), &(loop->active_requests[i]), sizeof(uv_request_t*));
        }
    }

    loop->n_active_requests = new_n_active;
    loop->active_requests = new_pointer;

    return 0;
}

int
uv_insert_tcp(uv_tcp_t* tcp, uv_request_t* req, tcp_type type){
    int n_active;
    uv_request_t** pointer;

    switch (type)
    {
    case CONNECT:
        n_active = tcp->n_connect_requests;
        pointer = (uv_request_t**)tcp->connect_requests;
        break;

    case LISTEN:
        n_active = tcp->n_listen_requests;
        pointer = (uv_request_t**)tcp-> listen_requests;
        break;

    case ACCEPT:
        n_active = tcp->n_accept_requests;
        pointer = (uv_request_t**)tcp->accept_requests;
        break;

    case READ_START:
        n_active = tcp->n_read_start_requests;
        pointer = (uv_request_t**)tcp->read_start_requests;
        break;

    case READ_STOP:
        n_active = tcp->n_read_stop_requests;
        pointer = (uv_request_t**)tcp->read_stop_requests;
        break;

    case WRITE:
        n_active = tcp->n_write_requests;
        pointer = (uv_request_t**)tcp->write_requests;
        break;
    
    default:
        return 1;
    }

    int i = n_active++; // array index

    if(i == 1){
        pointer = malloc(sizeof(uv_request_t*));
        if(!pointer){
            ESP_LOGE("UV_INSERT_REQUESTS", "Error pointer is NULL after malloc in uv_insert_requests");
            return 1;
        }
            
        memcpy(pointer, &req, sizeof(uv_request_t*));
    } else{
        pointer = realloc(pointer, i * sizeof(uv_request_t*));
        if(!pointer){
            ESP_LOGE("UV_INSERT_REQUESTS", "Error pointer is NULL after malloc in uv_insert_requests");
            return 1;
        }
        memcpy(&(pointer[i-1]), &req, sizeof(uv_request_t*));
    }

    switch (type)
    {
    case CONNECT:
        tcp->n_connect_requests = i;
        tcp->connect_requests = pointer;
        break;

    case LISTEN:
        tcp->n_listen_requests = i;
        tcp-> listen_requests = pointer;
        break;

    case ACCEPT:
        tcp->n_accept_requests = i;
        tcp->accept_requests = pointer;
        break;

    case READ_START:
        tcp->n_read_start_requests = i;
        tcp->read_start_requests = pointer;
        break;

    case READ_STOP:
        tcp->n_read_stop_requests = i;
        tcp->read_stop_requests = pointer;
        break;

    case WRITE:
        tcp->n_write_requests = i;
        tcp->write_requests = pointer;
        break;
    
    default:
        return 1;
    }

    return 0;
}

int
uv_remove_tcp(uv_tcp_t* tcp, uv_request_t* req, tcp_type type){
    int n_active;
    uv_request_t** pointer;

    switch (type)
    {
    case CONNECT:
        n_active = tcp->n_connect_requests;
        pointer = tcp->connect_requests;
        break;

    case LISTEN:
        n_active = tcp->n_listen_requests;
        pointer = tcp-> listen_requests;
        break;

    case ACCEPT:
        n_active = tcp->n_accept_requests;
        pointer = tcp->accept_requests;
        break;

    case READ_START:
        n_active = tcp->n_read_start_requests;
        pointer = tcp->read_start_requests;
        break;

    case READ_STOP:
        n_active = tcp->n_read_stop_requests;
        pointer = tcp->read_stop_requests;
        break;

    case WRITE:
        n_active = tcp->n_write_requests;
        pointer = tcp->write_requests;
        break;
    
    default:
        return 1;
    }

    int new_n_active = n_active - 1;
    uv_request_t** new_pointer = malloc(new_n_active * sizeof(uv_request_t*));

    if(!new_pointer){
        ESP_LOGE("UV_REMOVE_REQUESTS", "Error new_pointer is NULL after malloc in uv_remove_requests");
        return 1;
    }

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < n_active; i++){
        if(pointer[i] != req){
            memcpy(&(new_pointer[j++]), &(pointer[i]), sizeof(uv_request_t*));
        }
    }

    switch (type)
    {
    case CONNECT:
        tcp->n_connect_requests = new_n_active;
        tcp->connect_requests = new_pointer;
        break;

    case LISTEN:
        tcp->n_listen_requests = new_n_active;
        tcp-> listen_requests = new_pointer;
        break;

    case ACCEPT:
        tcp->n_accept_requests = new_n_active;
        tcp->accept_requests = new_pointer;
        break;

    case READ_START:
        tcp->n_read_start_requests = new_n_active;
        tcp->read_start_requests = new_pointer;
        break;

    case READ_STOP:
        tcp->n_read_stop_requests = new_n_active;
        tcp->read_stop_requests = new_pointer;
        break;

    case WRITE:
        tcp->n_write_requests = new_n_active;
        tcp->write_requests = new_pointer;
        break;
    
    default:
        return 1;
    }

    return 0;
}