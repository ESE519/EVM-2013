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
//#include "messageTypes.h"
//#include "function_manager.h"
#include "UtsavAPI.h"
#include "packethandler.h"

int globalIndex=0;  //index of the send buffer array  
char slavesList[MAX_SLAVES];


void check_and_send_node_died_signal(int k);       //defined in senderNode.cpp  

void set_MessageTypes(uint8_t* recdata,int messageName){
	
	globalIndex=0;
	recdata[globalIndex]= (uint8_t) messageName;
	globalIndex++;
	
}

void resetIndex(){
	globalIndex=0;
}

void set_FuncCode(uint8_t* recdata,const char *name, uint16_t sizel, char *code){
	int i;
	
	memcpy(&recdata[globalIndex],&sizel,2);
	globalIndex = globalIndex + 2;

	strcpy((char *)&(recdata[globalIndex]), name);
	globalIndex += strlen(name) + 1;

	for (i=0;i<sizel;i++)
		recdata[globalIndex++] = code[i];
	
	resetIndex();
}

void set_TaskParams(uint8_t* recdata,uint16_t psecs,uint16_t p_ms,uint16_t wsec,uint16_t wms){
	memcpy(&recdata[globalIndex], &psecs, 2);
	globalIndex += 2;
	
	memcpy(&recdata[globalIndex], &p_ms, 2);
	globalIndex += 2;
	
	memcpy(&recdata[globalIndex], &wsec, 2);
	globalIndex += 2;
	
	memcpy(&recdata[globalIndex], &wms, 2);
	globalIndex += 2;
	resetIndex();

}

void set_number_of_functions(uint8_t* recdata,uint8_t number){
	recdata[globalIndex] = number;
	globalIndex++;
}
void set_functionNames(uint8_t* recdata, const char *functionName){
	strcpy((char *)recdata + globalIndex, functionName);
	globalIndex += strlen(functionName) + 1;
	
}


void set_States(uint8_t* recdata,uint32_t *state){
	memcpy(&recdata[globalIndex], state, sizeof(uint32_t) * 10);
	resetIndex();
}

void set_activate(uint8_t* recdata,const char *taskName){
	strcpy((char *)&recdata[globalIndex], taskName);
	resetIndex();
}

void set_deactivate(uint8_t* recdata,uint8_t taskName){
	recdata[globalIndex++] = taskName;
	resetIndex();
}

void checkPings(int recFrom){
	slavesList[recFrom - 2]=10;
}

void reduceTTL() {
	for(int k=0;k<MAX_SLAVES;k++){
		if(slavesList[k]!=0)
				slavesList[k]--;
		
		if(slavesList[k] == 0)
			check_and_send_node_died_signal(k+2);
	}
}

