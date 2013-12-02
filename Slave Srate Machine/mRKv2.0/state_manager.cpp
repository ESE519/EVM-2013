#include "state_manager.h"
#include "fn_task_map.h"

uint32_t states[MAX_TASKS][MAX_NUM_STATES];



int get_state(int task_num, uint8_t pos, uint32_t *ptr) {
		
	if(task_num < 0) {
		printf("Incorrect task num\n\r");
		return -1;
	}
	
	if(pos > MAX_NUM_STATES) {
		printf("Pos argument exceeds maximum number of states\n\r");
		return -1;
	}
	
	*ptr = states[task_num][pos];
	return 0;
	
}






int checkpoint_state(int task_num, uint8_t pos, uint32_t val) {
	if(task_num < 0) {
		printf("Incorrect task num\n\r");
		return -1;
	}
	
	if(pos > MAX_NUM_STATES) {
		printf("Pos argument exceeds maximum number of states\n\r");
		return -1;
	}
	
	states[task_num][pos] = val;
	return 0;
}


int store_states(int task_num, uint32_t *data) {
	if(task_num < 0) {
		printf("Incorrect task num\n\r");
		return -1;
	}
	
	
	memcpy(states[task_num], data, sizeof(uint32_t) * 10);
	return 0;
}
