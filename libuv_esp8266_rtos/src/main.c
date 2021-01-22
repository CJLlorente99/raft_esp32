/* This file should contain some code that verifies libuv implementation correctness
To be verified
    signals -> some buttons and led (when one is pressed all leds on, when the other is pressed all leds off)

*/

#include "uv.h"

#define INPUT_TEST_PORT 4
#define LED_TEST_PORT 5

void
test_callback (uv_signal_t* handle, int signum){
    if(GPIO_INPUT_GET(signum) == 0){
        GPIO_OUTPUT_SET(signum, 0);
    } else {
        GPIO_OUTPUT_SET(signum, 1);
    }
}

void
main(void* ignore){

    // Configure GPIO
    GPIO_AS_OUTPUT(LED_TEST_PORT);
    GPIO_AS_INPUT(INPUT_TEST_PORT);

    // Init loop
    uv_loop_t* loop;

    if(uv_loop_init(loop)){
        // do something because error has been caused
    }

    // Init signal handle
    uv_signal_t* signal_handle;

    if(uv_signal_init(loop, signal_handle)){
        // do something because error has been caused
    }

    if(uv_signal_start(signal_handle, test_callback, INPUT_TEST_PORT)){
        // do something because error has been caused
    }

    while(true){
        vTaskDelay(1000/LOOP_RATE_MS);
        fsm_fire(loop->loopFSM);
    }

    vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    xTaskCreate(&main, "startup", 2048, NULL, 1, NULL);
}