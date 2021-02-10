#ifndef _TREE_FSM_
#define _TREE_FSM_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "esp_common.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

#include "gpio.h"

#include "fsm.h"

// Useful constants
#define  BLUE_LED 4
#define YELLOW_LED 0
#define GREEN_LED 2

// Flags
typedef struct FlagsType
{
	unsigned int color; // only three possible colors 01->blue 10->yellow 11->green
}FlagsType;

//Fsm needed data
typedef struct TreeFSM
{
	unsigned int color_shining : 2;
}TreeFSM;

// Function declaration

fsm_t* fsm_new_treeFSM ();
void new_random_color();
int updateTreeFSM(TreeFSM* fsm, TreeFSM* newData);
int fillSnapshot(TreeFSM* fsm, TreeFSM* newData);

#endif /* _TREE_FSM_ */