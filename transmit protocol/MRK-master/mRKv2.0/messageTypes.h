#include <include.h>
#include <ulib.h>
#include <stdio.h>

#define DESTINATION_ADDRESS_LOCATION 0  //Defines the destination address location
#define SOURCE_ADDRESS_LOCATION 1				//Defines the location of the source address	
#define SEQUENCE_NUM_LOCATION 2				 //Defines the location at which the sequence number is present	
#define MESSAGE_TYPE_LOCATION 3 			 // Defines the location of the message type
#define PACKET_NUM_LOCATION 4 				 //Defines the location where we tell if its first/last or intermediate packet
#define DATA_START_LOCATION 5					 //Defines the start of the data location

//Defines the maximum number of nodes 
#define MAX_MOLES  4


//Define the message types associated with the lowest level 
enum messageTypes{
    DATA =1,
    DATA_PACKET_ACK = 2
};






//Message format 

/**********************************************
Byte No 								DATA 
1												Destination ID 
2												Source ID 
3												Sequence Number 
4												Packet Type Eg. TASK , STACK etc 
5 											Packet Number - 1st , 2nd ???????
6-105 									Data
*************************************************/


