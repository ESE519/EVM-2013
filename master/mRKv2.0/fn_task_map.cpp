#include "fn_task_map.h"
#include "function_manager.h"


char fn_task_map[MAX_TASKS][MAX_NAME_LENGTH];

uint8_t current_available_task = 0;



int map_function_task(const char *name) {
	
	if(current_available_task >= MAX_TASKS)
		return -1;
	
	strcpy(fn_task_map[current_available_task], name);
	current_available_task++;
	
	return 0;
}



int get_task_from_function(const char *name) {
	int i;
	
	for(i = 0; i < MAX_TASKS; i++) {
		if(strcmp(name, fn_task_map[i]) == 0)
			return i;
	}
	
	return -1;
}


