#include <include.h>
#include <ulib.h>
#include <stdio.h>

#define DESTINATION_ADDRESS_LOCATION 0 //Defines the destination address location
#define SOURCE_ADDRESS_LOCATION 1
#define SEQUENCE_NUM_LOCATION 2
#define MESSAGE_TYPE_LOCATION 3 // Defines the location of the message type
#define PACKET_NUM_LOCATION 4 //Defines the location where we tell if its first/last or intermediate packet
#define DATA_START_LOCATION 5//Defines the start of the data location

/*********************************************************************************************************************************************/
  #define DATA_TYPE 1
  #define TASK 1
	#define TASK_PACKET_ACK 2
	#define PING 3
	#define TASK_PARAMS  4
	#define FUNC_NAMES 5
	#define FUNC_CODE 6
	#define INIT_STATES 7
	#define CHECKPOINT_STATES 8
	#define TASK_ACTIVATE 9
	#define TASK_DEACTIVATE 10
	#define ACK 11
	#define NACK 12
/*
enum messageTypes{
	TASK =1,
	TASK_PACKET_ACK = 2,
	
	PING=3,
	TASK_PARAMS =4,
	FUNC_NAMES=5,
	FUNC_CODE=6,
	INIT_STATES=7,
	CHECKPOINT_STATES=8,
	TASK_ACTIVATE=9,
	TASK_DEACTIVATE=10,
	ACK=11,
	NACK=12
};


*/



//Message format 

/**********************************************
Byte No 								DATA 
1												Destination ID 
2												Source ID 
3												Sequence Number 
4												Packet Type Eg. TASK , STACK etc 
5 											Packet Number - 1st , 2nd ???????



*************************************************/


