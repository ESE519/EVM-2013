#include "function_manager.h"

#define FUNCTION_TABLE_SIZE		50

/*each function_info is 24 bytes. Storing 50 such will consume 1200 bytes. 
This can be made dynamic. Need to discuss the pros and cons */

struct function_info		function_table[FUNCTION_TABLE_SIZE]; 
unsigned int current_num_functions = 0;
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
	
	//printf("finding %s\n\r", name);
	
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
	


