#include "states.h"

LOW_LEVEL_STATE low_state = IDLE_LOW;
LOW_LEVEL_SIGNAL low_sig = NO_SIGNAL_LOW;

void send_low_level_signal (LOW_LEVEL_SIGNAL sig) {
	low_sig = sig;
}


void low_level_take_action () {
	
	switch(low_state) {
		case IDLE:
		switch(low_sig) {
			case SEND_TASK_LOW:
				//send_task_params();
			low_state = TASK_PARAMS;
			low_sig = NO_SIGNAL_LOW;
			break;
			
			default:
			break;
		}
		break;
		
		
		
		case TASK_PARAMS:
		switch(low_sig) {
			case ACK:
				//send function names
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
    if(low_sig == ACK) {
			//send function code.
			low_state = FUNC_CODE;
			low_sig = NO_SIGNAL_LOW;
		}
		
		break;
		
		
		
		
		case FUNC_CODE:
		if(low_sig == ACK) {
			//if all functions code sent... then
			//send init state.
			low_state = INIT_STATE;
			low_sig = NO_SIGNAL_LOW;
			//if some functions code still left.
			//send next function.
		}
		break;
		
		case INIT_STATE:
		if(low_sig == ACK) {
			//send deactivate signal to the current node with the task.
			low_state = DEACTIVATE;
			low_sig = NO_SIGNAL_LOW;
		}
		break;
		
		
		
		
		case DEACTIVATE:
		if(low_sig == ACK) {
			//send activate signal to the node.
			low_state = ACTIVATE;
			low_sig = NO_SIGNAL_LOW;
		}
		break;
		
		
		
		case ACTIVATE:
		if(low_sig == ACK) {
			//done transferring the current task.
			send_middle_level_signal(SENDING_DONE);
			low_state = IDLE_LOW;
			low_sig = NO_SIGNAL_LOW;
		}
		break;
		
			
	}
}
		
			
