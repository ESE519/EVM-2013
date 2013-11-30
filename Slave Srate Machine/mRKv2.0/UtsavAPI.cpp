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
uint16_t pinsecs,pinms,winsec,winms;


void set_MessageTypes(uint8_t* recdata,int messageName){
	globalIndex=0;
	recdata[globalIndex]= messageName;
	globalIndex++;
}

void resetIndex(){
	globalIndex=0;
}

void set_FuncCode(uint8_t* recdata,const char *name, uint16_t size, char *code){
	memcpy(recdata+1,&size,2);
	globalIndex = globalIndex + 2;
	strcpy((char *)recdata + globalIndex, name);
	globalIndex += strlen(name) + 1;
	
	for (int i=0;i<size;i++)
		recdata[globalIndex++] = code[i];
	resetIndex();
}

void set_TaskParams(uint8_t* recdata,uint16_t psecs,uint16_t p_ms,uint16_t wsec,uint16_t wms){
	memcpy(recdata+1,&psecs,2);
	memcpy(recdata+3,&p_ms,2);
	memcpy(recdata+5,&wsec,2);;
	memcpy(recdata+7,&wms,2);;
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

void set_activate(uint8_t* recdata,const char *taskName){
	strcpy((char*)recdata+globalIndex,taskName);
	resetIndex();
}

void set_deactivate(uint8_t* recdata,const char *taskName){
	strcpy((char*)recdata+globalIndex,taskName);
	resetIndex();
}


void set_FuncReply(uint8_t* recdata, uint16_t data) {
	memcpy(&recdata[globalIndex], &data, 2);
	resetIndex();
}


void checkPings(int recFrom){
	slavesList[SOURCE_ADDRESS_LOCATION]=10;
	for(int k=1;k<MAX_SLAVES;k++){
		if(slavesList[recFrom]!=0)
				slavesList[recFrom]--;
	}
}




void getTaskParams(uint8_t* recdata){
	pinsecs=recdata[1];
	pinsecs <<= 8;
	pinsecs |= recdata[2];
	
	pinms=recdata[3];
	pinms <<= 8;
	pinms |= recdata[4];
	
	winsec=recdata[5];
	winsec <<= 8;
	winsec |= recdata[6];
	
	winms=recdata[7];
	winms <<= 8;
	winms |= recdata[7];
}

void getFuncNames(uint8_t* recdata){
	
}

void getFuncCode(uint8_t* recdata){
	
}

void getNumberofStates(uint8_t* recdata){
	
	
}
void getInitiialStates(uint8_t* recdata){
	
}

void sendCheckPointStates(uint8_t* recdata){
	
}


