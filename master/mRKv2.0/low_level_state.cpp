#include "states.h"
#include "function_manager.h"
#include "UtsavAPI.h"
#include "transmitProtocol.h"

#define DATA_LOCATION(ADD) ((ADD-1)*512)

LOW_LEVEL_STATE low_state = IDLE_LOW;
LOW_LEVEL_SIGNAL low_sig = NO_SIGNAL_LOW;
extern int node_to_send;   //the node to send the data to
extern uint8_t data[512*MAX_MOLES];      //sumukh's array to write the tx packet to.
extern struct task_function_info task_function_table[MAX_NUM_TASKS_IN_NETWORK];

/************* private function definitions *************/
void send_task_params();
void send_function_names();
void send_task_function_code();
void send_function_code();
int any_functions_left();
/*******************************************************/


/************ private variables ***********************/
int num_functions_sent = 0;
/******************************************************/

void send_low_level_signal (LOW_LEVEL_SIGNAL sig) {
	low_sig = sig;
}


void low_level_take_action () {
	
	switch(low_state) {
		case IDLE:
		printf("idle low level\n\r");
		switch(low_sig) {
			case SEND_TASK_LOW:
			send_task_params();                 //done
			low_state = TASK_PARAMS;
			low_sig = NO_SIGNAL_LOW;
			break;
			
			default:
			break;
		}
		break;
		
		
		
		case TASK_PARAMS:
		printf("Task params\n\r");
		switch(low_sig) {
			case ACK:
			send_function_names();
			low_state = FUNC_NAMES;
			low_sig = NO_SIGNAL_LOW;
			break;
		
			case NACK:
				//node did not accept... find different node.
			send_middle_level_signal(SENDING_DONE);
			low_state = IDLE_LOW;
			low_sig = NO_SIGNAL_LOW;
			break;
		}
		break;
		
		
		
		
		
		case FUNC_NAMES:
		printf("Func names\n\r");
    if(low_sig == ACK) {
			num_functions_sent = 0;
			send_task_function_code();
			low_state = FUNC_CODE;
			low_sig = NO_SIGNAL_LOW;
		}
		
		break;
		
		
		
		
		case FUNC_CODE:
		printf("Func code\n\r");
		if(low_sig == ACK) {
			//if all functions code sent... then
			//send init state.
			if(!any_functions_left()) {
				low_state = INIT_STATE;
				low_sig = NO_SIGNAL_LOW;
			}
			//if some functions code still left.
			//send next function.
			else {
				send_function_code();
				low_state = FUNC_CODE;
				low_sig = NO_SIGNAL_LOW;
			}
		}
		break;
		
		case INIT_STATE:
		printf("init state\n\r");
		if(low_sig == ACK) {
			//send deactivate signal to the current node with the task.
			low_state = DEACTIVATE;
			low_sig = NO_SIGNAL_LOW;
		}
		break;
		
		
		
		
		case DEACTIVATE:
		printf("Deactivate\n\r");
		if(low_sig == ACK) {
			//send activate signal to the node.
			low_state = ACTIVATE;
			low_sig = NO_SIGNAL_LOW;
		}
		break;
		
		
		
		case ACTIVATE:
		printf("Activate\n\r");
		if(low_sig == ACK) {
			//done transferring the current task.
			send_middle_level_signal(SENDING_DONE);
			low_state = IDLE_LOW;
			low_sig = NO_SIGNAL_LOW;
		}
		break;
		
			
	}
}
		
			




void send_task_params() {
	int task_num, ret_val;
	struct task_function_info *task_ptr;
	
	task_num = find_first_unassigned_task();
	
	task_ptr = &(task_function_table[task_num]);
	printf("task name %s\n\r", task_ptr->name);
	printf("task parameters %d %d %d %d \n\r", task_ptr->periods, task_ptr->periodms, task_ptr->wcets, task_ptr->wcetms);
	set_MessageTypes(&data[DATA_LOCATION(node_to_send)], TYPE_TASK_PARAMS);
	
	set_TaskParams(&data[DATA_LOCATION(node_to_send)], 
								 task_ptr->periods,
	               task_ptr->periodms,
	               task_ptr->wcets,
	               task_ptr->wcetms);
	printf("done sending\n\r");
	if(!ReadyToSendData(node_to_send))
		printf("Error: Ready to send data returned 0\n\r");
	
	startDataTransmission(node_to_send, 25);     //acutal no of bytes is just 17
	
}
	




void send_function_names() {
	int task_num, ret_val, i, length;
	struct task_function_info *task_ptr;
	task_num = find_first_unassigned_task();
	task_ptr = &task_function_table[task_num];
	
	set_MessageTypes(&data[DATA_LOCATION(node_to_send)], TYPE_FUNC_NAMES);
	set_number_of_functions(&data[DATA_LOCATION(node_to_send)], task_ptr->num_references);
	for(i = 0; i < task_ptr->num_references; i++) {
		set_functionNames(&data[DATA_LOCATION(node_to_send)], task_ptr->fun_references[i]);
	}
	
	if(!ReadyToSendData(node_to_send) )
		printf("Error: Ready to send data returned 0 \n\r");
	
	length = 2 + MAX_NAME_LENGTH * task_ptr->num_references;
	startDataTransmission(node_to_send, length);
}




void send_task_function_code() {
	int task_num, ret_val, i, length;
	struct task_function_info *task_ptr;
	const char *fun_name;
	char *fn_ptr;
	task_num = find_first_unassigned_task();
	task_ptr = &task_function_table[task_num];
	
	fun_name = task_ptr->name;
	printf("in sending code, name is %s\n\r", fun_name);
	length = 256;         //To be done... put correct function size.
	fn_ptr = (char *) get_function_handle(fun_name, MAX_NAME_LENGTH - 1);
	fn_ptr = (char *)((uint32_t)fn_ptr & ~(1));
	
	
	set_MessageTypes(&data[DATA_LOCATION(node_to_send)], TYPE_FUNC_CODE);
	set_FuncCode(&data[DATA_LOCATION(node_to_send)], fun_name, length, fn_ptr);
	
	if(!ReadyToSendData(node_to_send) )
		printf("Error: Ready to send data returned 0 \n\r");
	
	length = 3 + MAX_NAME_LENGTH + 256;
	startDataTransmission(node_to_send, length);
	
	
}





void send_function_code() {
	int task_num, ret_val, i, length;
	struct task_function_info *task_ptr;
	const char *fun_name;
	char *fn_ptr;
	task_num = find_first_unassigned_task();
	task_ptr = &task_function_table[task_num];
	
	fun_name = task_ptr->fun_references[num_functions_sent];
	length = 256;         //To be done... put correct function size.
	fn_ptr = (char *) get_function_handle(fun_name, MAX_NAME_LENGTH - 1);
	fn_ptr = (char *)((uint32_t)fn_ptr & ~(1));
	
	set_MessageTypes(data, TYPE_FUNC_CODE);
	set_FuncCode(data, fun_name, length, fn_ptr);
	
	if(!ReadyToSendData(node_to_send) )
		printf("Error: Ready to send data returned 0 \n\r");
	
	length = 3 + MAX_NAME_LENGTH + 256;
	startDataTransmission(node_to_send, length);
	
	num_functions_sent++;
}



	
int any_functions_left() {
	int task_num, ret_val, i, length;
	struct task_function_info *task_ptr;
	const char *fun_name;
	char *fn_ptr;
	task_num = find_first_unassigned_task();
	task_ptr = &task_function_table[task_num];
	
	if(num_functions_sent == task_ptr->num_references) {
		printf("no more functions left\n\r");
		return 0;
	}
	
	else {
		printf("Functions left is %d\n\r", task_ptr->num_references - num_functions_sent);
		return 1;
	}
}
