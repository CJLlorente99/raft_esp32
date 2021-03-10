#include "uv.h"

#define LED_DEBUG_PORT 5

// TODO
// Creo que esto no esta bien. Debería ser void*** pointer?

int
uv_insert(void*** pointer, int* active, size_t size, void* handle){
    int i = *active++; // array index

    if(i == 1){
        *pointer = malloc(size);
        if(!*pointer){
            ESP_LOGE("uv_insert_HANDLE", "Error pointer is NULL after malloc in uv_insert_handle");
            return 1;
        }
            
        memcpy(pointer, &handle, size);
    } else{
        *pointer = realloc(pointer, i * size);
        if(!*pointer){
            ESP_LOGE("uv_insert_HANDLE", "Error pointer is NULL after malloc in uv_insert_handle");
            return 1;
        }
        memcpy(pointer[i-1], &handle, size);
    }

    *active = i;

    return 0;
}

int
uv_remove(void*** pointer, int* active, size_t size, void* handle){
    // Allocate memory for new array of handlers
    int new_n_active = *active - 1;
    void** new_pointer = malloc(new_n_active * size);

    if(!new_pointer){
        ESP_LOGE("uv_remove_HANDLE", "Error new_pointer is NULL after malloc in uv_remove_handles");
        return 1;
    }

    // Add handlers, except from the one stopped
    int j = 0;
    for(int i = 0; i < *active; i++){
        if(pointer[i] != handle){
            memcpy(new_pointer[j++], &(pointer[i]), size);
        }
    }

    *active = new_n_active;
    *pointer = new_pointer;

    return 0;
}