#include "treeFSM.h"
#include "uv.h"

// Variables
static FlagsType flags;

// FSM states
enum states {
	IDLE,
	SHINING
};

// Checking functions
static int
check_blue (fsm_t* this)
{	
	if (flags.color == 1){
		return 1;
	} else{
		return 0;
	}
}

static int
check_yellow (fsm_t* this)
{	
	if (flags.color == 2){
		return 1;
	} else{
		return 0;
	}
}

static int
check_green (fsm_t* this)
{	
	if (flags.color == 3){
		return 1;
	} else{
		return 0;
	}
}

// FSM functions
static void
blue_on (fsm_t* this)
{
	TreeFSM *p_treeFSM;
	p_treeFSM = (TreeFSM*)(this->user_data);

	p_treeFSM->color_shining = 1;
    ESP_LOGI("user_FSM", "color shining is %d", p_treeFSM->color_shining);

	flags.color = 0;

}

static void
yellow_on (fsm_t* this)
{
	TreeFSM *p_treeFSM;
	p_treeFSM = (TreeFSM*)(this->user_data);

	p_treeFSM->color_shining = 2;
    ESP_LOGI("user_FSM", "color shining is %d", p_treeFSM->color_shining);

	flags.color = 0;

}

static void
green_on (fsm_t* this)
{
	TreeFSM *p_treeFSM;
	p_treeFSM = (TreeFSM*)(this->user_data);

	p_treeFSM->color_shining = 3;
    ESP_LOGI("user_FSM", "color shining is %d", p_treeFSM->color_shining);

	flags.color = 0;

}

// FSM init

fsm_t* fsm_new_treeFSM ()
{
	static fsm_trans_t treeFSM_tt[] = {

		{ IDLE, check_blue, SHINING, blue_on},
        { IDLE, check_yellow, SHINING, yellow_on},
        { IDLE, check_green, SHINING, green_on},
        { SHINING, check_blue, SHINING, blue_on},
        { SHINING, check_yellow, SHINING, yellow_on},
        { SHINING, check_green, SHINING, green_on},
		{ -1, NULL, -1, NULL},
	};

	flags.color = 0;

    TreeFSM* treeFSM = malloc(sizeof(TreeFSM));
    treeFSM->color_shining = 0;

	srand(1);

	return fsm_new (IDLE, treeFSM_tt, treeFSM);
}

// Function to change to random color

void
new_random_color (){
	int r = (rand() % 3) + 1; // random number between 1 and 3
	flags.color = r;
}