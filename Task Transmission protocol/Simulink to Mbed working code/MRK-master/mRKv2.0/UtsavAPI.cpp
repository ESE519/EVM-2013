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

char globalIndex=0;  //index of the send buffer array  
char slavesList[MAX_SLAVES];

void set_MessageTypes(uint8_t* recdata,int messageName){
	
	recdata[globalIndex]= messageName;
	globalIndex++;
	
	printf("%d",recdata[0]);
}

void resetIndex(){
	globalIndex=0;
}

void set_FuncCode(uint8_t* recdata,char* name, uint16_t size, char *code){
	recdata[globalIndex++] = *name;
	memcpy(recdata+2,&size,2);
	for (int i=0;i<size;i++)
		recdata[globalIndex] = code[i];
	resetIndex();
}

void set_TaskParams(uint8_t* recdata,uint16_t psecs,uint16_t p_ms,uint16_t wsec,uint16_t wms){
	recdata[globalIndex++] = psecs;
	recdata[globalIndex++]= p_ms;
	recdata[globalIndex++]= wsec;
	recdata[globalIndex++]= wms;
	resetIndex();

}

void set_number_of_functions(uint8_t* recdata,uint8_t number){
	recdata[globalIndex] = number;
	globalIndex++;
}
void set_functionNames(uint8_t* recdata,char *functionName,int number){
	if(globalIndex < number)
			recdata[globalIndex++] = *functionName;
	if(globalIndex+1== globalIndex)
			resetIndex();
}
void set_numofStates(uint8_t* recdata,uint8_t number){
	recdata[globalIndex++] = number;
	
}

void set_States(uint8_t* recdata,uint32_t state,int number){
	if(globalIndex < number)
		memcpy(recdata+globalIndex,&state,4);
	globalIndex+=4;
	if(globalIndex+1 == number)
		resetIndex();
}

void set_activate(uint8_t* recdata,char* taskName){
	recdata[globalIndex++] = *taskName;
	resetIndex();
}

void set_deactivate(uint8_t* recdata,char* taskName){
	recdata[globalIndex++] = *taskName;
	resetIndex();
}

void checkPings(int recFrom){
	slavesList[SOURCE_ADDRESS_LOCATION]=10;
	for(int k=1;k<MAX_SLAVES;k++){
		if(slavesList[recFrom]!=0)
				slavesList[recFrom]--;
	}
}

