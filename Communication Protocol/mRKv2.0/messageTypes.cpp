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
#include "messageTypes.h"
#include "function_manager.h"

#define MAX_SLAVES 4

char send_buf[RF_MAX_PAYLOAD_SIZE];
char globalIndex=0;

void set_MessageTypes(char messageName){
	send_buf[globalIndex]= messageName;
	globalIndex++;
}

void resetIndex(){
	globalIndex=0;
}

void set_FuncCode(char *name, uint16_t size, char *code){
	send_buf[globalIndex] = *name;
	memcpy(send_buf+2,&size,2);
	for (int i=0;i<size;i++)
		send_buf[3+i] = code[i];
	resetIndex();
}

void set_TaskParams(uint16_t psecs,uint16_t p_ms,uint16_t wsec,uint16_t wms){
	send_buf[globalIndex] = psecs;
	send_buf[globalIndex+1]= p_ms;
	send_buf[globalIndex+1]= wsec;
	send_buf[globalIndex+1]= wms;
	resetIndex();
}

void set_number_of_functions(uint8_t number){
	send_buf[globalIndex] = number;
	globalIndex++;
}
void set_functionNames(char *functionName,int number){
	if(globalIndex < number)
			send_buf[globalIndex++] = *functionName;
	if(globalIndex+1== globalIndex)
			resetIndex();
}
void set_numofStates(uint8_t number){
	send_buf[globalIndex++] = number;
}

void set_States(uint32_t state,int number){
	if(globalIndex < number)
		memcpy(send_buf+globalIndex,&state,4);
	globalIndex+=4;
	if(globalIndex+1 == number)
		resetIndex();
}
void set_activate(char* taskName){
	send_buf[globalIndex] = *taskName;
	resetIndex();
}





char* Process_PingMessage(char *local_rx_buf,char *slavesList){
	 slavesList[local_rx_buf[0]] =10;
	 
return slavesList;
}


void checkforResponses(char *slavesList){
	for (int i=1;i<	MAX_SLAVES;i++)
			if(slavesList[i]!=0)
					slavesList[i]--;
}
