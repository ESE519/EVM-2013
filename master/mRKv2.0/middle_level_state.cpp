#include "states.h"
#include "function_manager.h"

MIDDLE_LEVEL_SIGNAL mid_sig = NO_SIGNAL_MIDDLE;
MIDDLE_LEVEL_STATE mid_state = IDLE;

void send_middle_level_signal (MIDDLE_LEVEL_SIGNAL sig) {
	mid_sig = sig;
};
	

void middle_level_take_action () {
	int val;
	
	switch(mid_state) {
		case IDLE:
		switch(mid_sig) {
			case FIND_NODE_SIGNAL:
				mid_state = FIND_NEXT_NODE;
		    mid_sig = NO_SIGNAL_MIDDLE;
				break;
			
			default:
				break;
		}
		break;
			
		case FIND_NEXT_NODE:
		find_next_node();
		send_low_level_signal(SEND_TASK_LOW);
		mid_state = SEND_TASK_MID;
		break;


		case SEND_TASK_MID:
		switch(mid_sig) {
			case SENDING_DONE:
			mid_sig = NO_SIGNAL_MIDDLE;
			val = find_unassigned_tasks();
			if(val == 0) {
				send_top_level_signal(ASSGN_DONE);
				mid_state = IDLE;
			}
			
			else {
				mid_state = FIND_NEXT_NODE;
			}
			break;
			
			default:
			break;
		}
		break;
	}
}

	