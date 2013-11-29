#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>
#include <string.h>
#include "mbed.h"
#include "basic_rf.h"
#include "bmac.h"
#include "slaveStates.h"
//#include "function_manager.h"
#include "messagetypes.h"
//#include "memory_manager.h"

States slaveState = IDLE_STATE;
Signals slaveSignal = NO_SIGNAL;


Signals slave_sm(uint8_t masterSignal){
	
		switch(slaveState){
			case IDLE_STATE:
				switch(masterSignal){
					case TYPE_TASK_PARAMS:
						slaveSignal=ACK_SIGNAL;
						slaveState=GOT_TASK_PARAM;
					  printf("Entered GOT_TASK_PARAM");
						break;
					default:
						slaveSignal=NACK_SIGNAL;
						break;
						}
				break;
			case GOT_TASK_PARAM:
					switch(masterSignal){
						case TYPE_FUNC_NAMES:
							slaveSignal = ACK_SIGNAL;
							slaveState = SAVED_FUNC_NAME;
							printf("Entered SAVED_FUNC_NAMES");
							break;
						default:
							slaveSignal = NACK_SIGNAL;
							break;
					}
				break;
			case SAVED_FUNC_NAME:
				switch(masterSignal){
					case TYPE_FUNC_CODE:
						slaveSignal = ACK_SIGNAL;
						slaveState = SAVED_FUNC_CODE;
					  printf("Entered Saved Func code");
						break;
				
					default:
							slaveSignal = NACK_SIGNAL;
							break;
				}
				break;
			case SAVED_FUNC_CODE:
				switch(masterSignal){
					case TYPE_INIT_STATES:
						slaveSignal = ACK_SIGNAL;
						slaveState = SAVED_INITIAL_STATES;
					  printf("Saved Initial States");
						break;
					
					default:
							slaveSignal = NACK_SIGNAL;
							break;
				}
			 break;
			case SAVED_INITIAL_STATES:
				switch(masterSignal){
					case TASK_ACTIVATE:
						slaveSignal = ACK_SIGNAL;
						slaveState = ACTIVATED_TASK;
						printf("Enterd Activated Task");
						break;
					
					default:
							slaveSignal = NACK_SIGNAL;
							break;
				}
			 break;
			}	
}
	
