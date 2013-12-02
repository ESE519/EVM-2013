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
#include "memory_manager.h"
#include "state_manager.h"

#define TX_LOCATION(ADD) ((ADD-1)*RF_MAX_PAYLOAD_SIZE)
#define DATA_LOCATION(ADD) ((ADD-1)*512)
#define MASTER_NODE 1
#define NUM_VIRTUAL_TASKS 	3

extern uint8_t receiveData[512*MAX_MOLES];			
extern uint16_t func_reply;
extern uint8_t current_task_num;
extern int (*virtual_functions[NUM_VIRTUAL_TASKS])(int);
extern uint8_t enable_virtual_tasks[NUM_VIRTUAL_TASKS];
extern nrk_task_type VIRTUAL_TASKS[NUM_VIRTUAL_TASKS];


States slaveState = IDLE_STATE;
uint16_t ps, pms, ws, wms;
uint32_t task_states[10];

uint32_t aligned_buffer[128];
char *task_function_ptr;

/********** FUNCTIONS ****************************/
void print_task_params();
void print_func_names();
void print_func_code();
void print_init_states();
int test_params();
void test_func_names();
void store_func_code(int task_code);
void activate_task();
int is_schedulable(uint16_t period_ms, uint16_t wcet_ms);    //code present in main.cpp

/*********************************************/

Signals slave_sm(uint8_t masterSignal){
	Signals slaveSignal = NO_SIGNAL;
	printf("master sig: %d\n\r", masterSignal);
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
						store_func_code(1);
						break;
				
					default:
							slaveSignal = NACK_SIGNAL;
							break;
				}
				break;
			case SAVED_FUNC_CODE:
				switch(masterSignal){
					case TYPE_FUNC_CODE:
						slaveSignal = ACK_SIGNAL;
						slaveState = SAVED_FUNC_CODE;
					  printf("Entered Saved Func code");
					  print_func_code();
					  store_func_code(0);
						break;
					
					case TYPE_INIT_STATES:
						slaveSignal = ACK_SIGNAL;
						slaveState = SAVED_INITIAL_STATES;
					  printf("Saved Initial States");
					  print_init_states();
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
						slaveState = IDLE_STATE;
						printf("Activating Task");
					  activate_task();
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
	int i;
	
	memcpy(&len, &receiveData[DATA_LOCATION(MASTER_NODE) + 1], 2);
	
	
	strcpy(name, (char *)&receiveData[DATA_LOCATION(MASTER_NODE) + 3]);
	namelen = strlen(name) + 1;
	printf("size of %s fn is %d\n\r", name, len);
	
	printf("\n\r");
	
}


void print_init_states() {
	
	int i;
	printf("init states: \n");
	memcpy(task_states, &receiveData[DATA_LOCATION(MASTER_NODE) + 1], sizeof(uint32_t) * 10);
  for(i = 0; i < 10; i++)
		printf("%d  ", task_states[i]);
	printf("\n\r");
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



void store_func_code(int task_code) {
	char name[20];
	uint16_t len, namelen;
	int i, err;
	char *fn_ptr;
	char *buffer;
	
	
	
	memcpy(&len, &receiveData[DATA_LOCATION(MASTER_NODE) + 1], 2);
	
	
	strcpy(name, (char *)&receiveData[DATA_LOCATION(MASTER_NODE) + 3]);
	namelen = strlen(name) + 1;
	
	buffer = (char *)aligned_buffer;
	memcpy(buffer, &receiveData[DATA_LOCATION(MASTER_NODE) + 3 + namelen], len);
	
	fn_ptr = copy_code_flash(buffer, len, &err);
	if(err != 0) {
		printf("error in IAP: %d\n\r", err);
	}
	
	else {
		printf("IAP Successful\n\r");
		err = function_register(name, namelen, (char *)((uint32_t)fn_ptr | 1));
		if(err)
			printf("error in fn reg\n\r");
	}
	//do something with fn ptr here.
	
	if(task_code) {
		printf("stored taskptr\n\r");
		task_function_ptr = (char *)((uint32_t) fn_ptr | 1);
	}
}




void activate_task() {
	VIRTUAL_TASKS[current_task_num].period.secs = ps;
	VIRTUAL_TASKS[current_task_num].period.nano_secs = pms * NANOS_PER_MS;
	VIRTUAL_TASKS[current_task_num].cpu_reserve.secs = ws;
	VIRTUAL_TASKS[current_task_num].cpu_reserve.nano_secs = wms * NANOS_PER_MS;
	
	int ret;
	int (*fp)(int);
	int (*test)();
	//test = (int (*)()) get_function_handle("test_ref2", 11);
	//printf("return from tst is %d\n\r", test());
	
	
	
	ret = store_states(0, task_states);
	if(ret) printf("error in storing states\n\r");
	
	fp = (int (*)(int))task_function_ptr;
	ret = fp(0);
	printf("Return value is %d\n\r", ret);
	
	virtual_functions[current_task_num] = ( int (*)(int) )task_function_ptr;
	//copy the states.
	enable_virtual_tasks[current_task_num] = 1;
	nrk_activate_task(&VIRTUAL_TASKS[current_task_num]);
	current_task_num++;
}
	