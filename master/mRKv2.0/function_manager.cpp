#include "function_manager.h"

#define FUNCTION_TABLE_SIZE		50

/*each function_info is 24 bytes. Storing 50 such will consume 1200 bytes. 
This can be made dynamic. Need to discuss the pros and cons */

struct function_info				function_table[FUNCTION_TABLE_SIZE]; 
struct task_function_info		task_function_table[MAX_NUM_TASKS_IN_NETWORK];

unsigned int current_num_functions = 0;
unsigned int current_num_task_functions = 0;

uint32_t address_of_get_handle __attribute__((at(GET_HANDLE_ADDRESS))); 

int function_register(const char *fn_name, int namelen, char *address){
	if( current_num_functions == FUNCTION_TABLE_SIZE ) {
		return -1;
	}
	
	if(namelen > MAX_NAME_LENGTH) {
		return -1;
	}
	
	strncpy(function_table[current_num_functions].name, fn_name, namelen);
	function_table[current_num_functions].fn_ptr = (void *) address;
	current_num_functions ++;
	
	return 0;
}


//After getting the function pointer from this function, it must be properly typecasted to the function pointer.

void * get_function_handle(const char *name, int namelen) {
	int i;
	
	printf("finding %s\n\r", name);
	
	if(namelen > MAX_NAME_LENGTH) {
		printf("FN HANDLE: len > max\n\r");
		return NULL;
	}
	
	for(i = 0; i < FUNCTION_TABLE_SIZE; i++) {
		if( strncmp(function_table[i].name, name, namelen) == 0 ) {
			return function_table[i].fn_ptr;
		}
	}
	
	printf("FN HANDLE: cant find fn\n\r");
	return NULL;
}
			
			
			
void function_manager_init() {
	address_of_get_handle = (uint32_t)&get_function_handle;
}
			


void print_all_function_names() {
	int i;
	for(i = 0; i < current_num_functions; i++)
		printf("%s\n\r", function_table[i].name);
}







int task_function_register(	const char *name, 
														int namelen, 
														char *address) {
	
	int ret_val, i;
															
	if( current_num_task_functions == MAX_NUM_TASKS_IN_NETWORK ) {
		return -1;
	}
	
	if(namelen > MAX_NAME_LENGTH) {
		return -1;
	}
	
	ret_val = function_register(name, namelen, address);
	
	if(ret_val != 0)
		return ret_val;
	
	strncpy(task_function_table[current_num_task_functions].name, name, namelen);
	task_function_table[current_num_task_functions].assigned_node_id = 0;
	task_function_table[current_num_task_functions].num_references = 0;
	 
}





int regsiter_reference(const char *task_name, const char *fun_name, int fn_namelen) {
	int i, ret_val, flag = 0, index;
	
	if(fn_namelen > MAX_NAME_LENGTH)
		return -1;
	
	for( i = 0; i < current_num_task_functions; i++) {
		if( strcmp(task_function_table[i].name, task_name) == 0) {
			flag = 1;
			break;
		}
	}
	
	if(!flag)
		return -1;
	
	
	index = task_function_table[i].num_references;
	if(index >= (MAX_REF_PER_TASK -1))
		return -1;
	
	
	strncpy(task_function_table[i].fun_references[index], fun_name, fn_namelen);
	(task_function_table[i].num_references)++;
}



int set_scheduling_parameters(const char *task_name, uint16_t ps, uint16_t pms, uint16_t es, uint16_t ems) {
	int i, flag = 0;
	for( i = 0; i < current_num_task_functions; i++) {
		if( strcmp(task_function_table[i].name, task_name) == 0) {
			flag = 1;
			break;
		}
	}
	
	if(!flag)
		return -1;
	
	task_function_table[i].periods = ps;
	task_function_table[i].periodms = pms;
	task_function_table[i].wcets = es;
	task_function_table[i].wcetms = ems;
	
	return 0;
}
	

	
	
int find_unassigned_tasks() {
	int i;
	for(i = 0; i < current_num_task_functions; i++) {
		if(task_function_table[i].assigned_node_id == 0) return 1;
			
	}
	
	return 0;
}



int find_first_unassigned_task() {
	int i;
	for(i = 0; i < current_num_task_functions; i++) {
		if(task_function_table[i].assigned_node_id == 0) return i;
	}
	
	return 0;
}


