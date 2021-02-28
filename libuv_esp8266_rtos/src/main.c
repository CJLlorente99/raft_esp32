/* This file should contain some code that verifies libuv implementation correctness
To be verified
    signals -> some buttons and led (when one is pressed all leds on, when the other is pressed all leds off)

*/

#include "uv.h"

// SIGNAL TEST

#define INPUT_TEST_PORT_OFF 15
#define INPUT_TEST_PORT_ON 4
#define LED_TEST_PORT 13
#define LED_DEBUG_PORT 5

void
test_callback_on (uv_signal_t* handle, int signum){
    GPIO_OUTPUT_SET(LED_TEST_PORT,1);
}

void
test_callback_off (uv_signal_t* handle, int signum){
    GPIO_OUTPUT_SET(LED_TEST_PORT,0);
}

void
main_signal(void* ignore){
    // Test led to be illuminated if execution begins
    GPIO_AS_OUTPUT(LED_DEBUG_PORT);
    GPIO_OUTPUT_SET(LED_DEBUG_PORT,0);

    // Configure GPIO
    GPIO_AS_OUTPUT(LED_TEST_PORT);
    GPIO_AS_INPUT(INPUT_TEST_PORT_ON);
    GPIO_AS_INPUT(INPUT_TEST_PORT_OFF);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        printf("Loop inicializado ERROR \n");
    }

    printf("Loop inicializado \n");

    // Init signal handle
    uv_signal_t* signal_handle_on = malloc(sizeof(uv_signal_t));
    uv_signal_t* signal_handle_off = malloc(sizeof(uv_signal_t));

    rv = uv_signal_init(loop, signal_handle_on);
    if(rv != 0){
        printf("Primera senal inicializada FALLO \n");
    }

    printf("Primera senal inicializada \n");

    // Aqui hay un error
    rv = uv_signal_start(signal_handle_on, test_callback_on, INPUT_TEST_PORT_ON);
    if(rv != 0){
        printf("Primera senal start FALLO \n");
    }

    printf("Primera senal start \n");

    rv = uv_signal_init(loop, signal_handle_off);
    if(rv != 0){
        printf("Segunda senal inicializada FALLO \n");
    }

    printf("Segunda senal inicializada \n");

    rv = uv_signal_start(signal_handle_off, test_callback_off, INPUT_TEST_PORT_OFF);
    if(rv != 0){
        printf("Segunda senal start FALLO \n");
    }

    printf("Segunda senal start \n");

    rv = uv_run(loop);
    if(rv != 0){
        // do something because error has been caused
    }

    vTaskDelete(NULL);
}

// TIMER TEST

// #define ONE_SHOT_CLOCK_TRIGGER_PORT 15
// #define LED_ONE_SHOT_PORT 17
// #define LED_TWO_SEC_PORT 18
#define LED_HALF_SEC_PORT 2

void
half_sec_callback_on (uv_timer_t* handle){
    GPIO_OUTPUT_SET(LED_HALF_SEC_PORT,1);
    printf("Encendiendo luz\n");
}

void
half_sec_callback_off (uv_timer_t* handle){
    GPIO_OUTPUT_SET(LED_HALF_SEC_PORT,0);
    printf("Apagando luz\n");
}

void
main_timer(void* ignore){   
    // Configure GPIO
    GPIO_AS_OUTPUT(LED_HALF_SEC_PORT);
    GPIO_AS_OUTPUT(LED_DEBUG_PORT);

    // Init loop
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    int rv;

    rv = uv_loop_init(loop);
    if(rv != 0){
        printf("Loop inicializado ERROR \n");
    }

    printf("Loop inicializado \n");

    // Init timers
    uv_timer_t* timer_half_sec_on = malloc(sizeof(uv_timer_t));
    uv_timer_t* timer_half_sec_off = malloc(sizeof(uv_timer_t));

    rv = uv_timer_init(loop, timer_half_sec_on);
    if(rv != 0){
        printf("Primer timer inicializado ERROR\n");
    }

    printf("Primer timer inicializado \n");

    rv = uv_timer_init(loop, timer_half_sec_off);
    if(rv != 0){
        printf("Segundo timer inicializado ERROR\n");
    }

    printf("Segundo timer inicializado\n");

    rv = uv_timer_start(timer_half_sec_on, half_sec_callback_on, 500, 1000);
    if(rv != 0){
        printf("Primer timer empezado ERROR\n");
    }

    printf("Primer timer empezado\n");

    rv = uv_timer_start(timer_half_sec_off, half_sec_callback_off, 1000, 1000);
    if(rv != 0){
        printf("Segundo timer empezado ERROR\n");
    }

    printf("Segundo timer empezado\n");

    rv = uv_run(loop);
    if(rv != 0){
        printf("Error durante uv_run\n");
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
uint32_t user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32_t rf_cal_sec = 0;
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
    // Enable GPIO interrupts
    // _xt_isr_unmask(1 << ETS_GPIO_INUM);

    vTaskDelay(5000/portTICK_RATE_MS);
    xTaskCreate(&main_timer, "startup", 2048, NULL, 5, NULL);
}