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
#include "function_manager.h"
//#include "memory_manager.h"

#define TX_LOCATION(ADD) ((ADD-1)*RF_MAX_PAYLOAD_SIZE)
#define DATA_LOCATION(ADD) ((ADD-1)*512)
#define MASTER_NODE 1


extern uint8_t receiveData[512*MAX_MOLES];			
extern uint16_t func_reply;

States slaveState = IDLE_STATE;
uint16_t ps, pms, ws, wms;


/********** FUNCTIONS ****************************/
void print_task_params();
void print_func_names();
void print_func_code();
int test_params();
void test_func_names();
int is_schedulable(uint16_t period_ms, uint16_t wcet_ms);    //code present in main.cpp

/*********************************************/

Signals slave_sm(uint8_t masterSignal){
	Signals slaveSignal = NO_SIGNAL;
	int ret_val;
		switch(slaveState){
			case IDLE_STATE:
				switch(masterSignal){
					case TYPE_TASK_PARAMS:
						//slaveSignal=ACK_SIGNAL;
						slaveState=GOT_TASK_PARAM;
					  printf("Entered GOT_TASK_PARAM");
					  //print_task_params();
					  ret_val = test_params();
					  if(ret_val) slaveSignal = ACK_SIGNAL;
					  else	slaveSignal = NACK_SIGNAL;
						break;
					default:
						slaveSignal=NACK_SIGNAL;
						break;
						}
				break;
			case GOT_TASK_PARAM:
					switch(masterSignal){
						case TYPE_FUNC_NAMES:
							slaveSignal = ACK_FUNC_NAMES;
							slaveState = SAVED_FUNC_NAME;
							printf("Entered SAVED_FUNC_NAMES");
						  //print_func_names();
						  test_func_names();
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
					  print_func_code();
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
return slaveSignal;
}
	


void print_task_params() {
	uint16_t ps, pms, ws, wms;
	printf("Task params:\n\r");
	memcpy(&ps, &receiveData[DATA_LOCATION(MASTER_NODE) + 1], 2);
	memcpy(&pms, &receiveData[DATA_LOCATION(MASTER_NODE) + 3], 2);
	memcpy(&ws, &receiveData[DATA_LOCATION(MASTER_NODE) + 5], 2);
	memcpy(&wms, &receiveData[DATA_LOCATION(MASTER_NODE) + 7], 2);
	
	printf("%d\t%d\t%d\t%d\n\r", ps, pms, ws, wms);
}



void print_func_names() {
	char name[20];
	int len, num_fn, i;
	
	num_fn = receiveData[DATA_LOCATION(MASTER_NODE) + 1];
	len = 1 + 1;
	
	printf("num funs received: %d", num_fn);
	printf("Function names received are:  ");
	for(i = 0; i < num_fn; i++) {
		strcpy(name, (char *)&receiveData[DATA_LOCATION(MASTER_NODE) + len]);
		len += strlen(name) + 1;
		printf("%s  ", name);
	}
}
  



void print_func_code() {
	char name[20];
	uint16_t len, namelen;
	
	memcpy(&len, &receiveData[DATA_LOCATION(MASTER_NODE) + 1], 2);
	
	
	strcpy(name, (char *)&receiveData[DATA_LOCATION(MASTER_NODE) + 3]);
	namelen = strlen(name) + 1;
	printf("size of %s fn is %d\n\r", name, len);
	
}
	


int test_params() {
	
	memcpy(&ps, &receiveData[DATA_LOCATION(MASTER_NODE) + 1], 2);
	memcpy(&pms, &receiveData[DATA_LOCATION(MASTER_NODE) + 3], 2);
	memcpy(&ws, &receiveData[DATA_LOCATION(MASTER_NODE) + 5], 2);
	memcpy(&wms, &receiveData[DATA_LOCATION(MASTER_NODE) + 7], 2);
	return is_schedulable(ps*1000 + pms, ws*1000 + wms);
}



void test_func_names() {
	char name[20];
	int len, num_fn, i, namelen;
	void *fn_ptr;
	
	num_fn = receiveData[DATA_LOCATION(MASTER_NODE) + 1];
	len = 1 + 1;
	func_reply = 0;
	for(i = 0; i < num_fn; i++) {
		strcpy(name, (char *)&receiveData[DATA_LOCATION(MASTER_NODE) + len]);
		namelen = strlen(name) + 1;
		len += namelen;
		fn_ptr = get_function_handle(name, namelen);
		
		if(fn_ptr == NULL) {
			printf("%s not present\n\r", name);
			func_reply |= (1 << i);
		}
		else{
			printf("%s already present\n\r", name);
		}
	}
}