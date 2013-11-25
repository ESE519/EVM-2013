/****************************************Utsav API**************************************************/

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

#define MAX_SLAVES 4
#define MASTER_ID 1

/** sets the type of message sent
    param : messageName - message type 
**/ 
void set_MessageTypes(uint8_t* recdata,int messageName);

/**
	sets up the message to send Function Code
	params: name- function name,
			size- function Size
			code- char array containing function code
**/
void set_FuncCode(uint8_t* recdata,uint8_t name, uint16_t size, char* code);

/**
	sets up the TASK PARAM messagte 
	params: psecs- Period (in secs)
			p_ms- period (in ms)
			wsec- WCET (in secs)
			wms-  WCET (in ms)

**/
void set_TaskParams(uint8_t* recdata,uint16_t psecs,uint16_t p_ms,uint16_t wsec,uint16_t wms);

/**
   sets no of functions to be sent
   params: number- number of functions sent
**/
void set_number_of_functions(uint8_t* recdata,uint8_t number);

/**
	sets the names of function in functionNames message
	params: number- number of functions sent
			function names - Name of the functions
**/
void set_functionNames(uint8_t functionName,int number);

/**
	sets number of states
	params: number - number of states
**/
void set_numofStates(uint8_t* recdata,uint8_t number);

/**
	sets the state bits in the state variable
	state : 32 bit state variable
	number : number of states		
**/
void set_States(uint8_t* recdata,uint32_t state,int number);

/**
	sends the activate signal for a task 
	params: taskName- name of task to be activated
**/
void set_activate(uint8_t* recdata,uint8_t* taskName);

/**
	sends the deactivate signal for a task 
	params: taskName- name of task to be activated
**/
void set_deactivate(uint8_t* recdata,uint8_t* taskName);

void checkPings(int recFrom);