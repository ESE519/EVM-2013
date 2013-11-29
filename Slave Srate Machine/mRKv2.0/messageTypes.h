#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

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
    #define TYPE_TASK_PARAMS  4
    #define TYPE_FUNC_NAMES 5
    #define TYPE_FUNC_CODE 6
    #define TYPE_INIT_STATES 7
    #define CHECKPOINT_STATES 8
    #define TASK_ACTIVATE 9
    #define TASK_DEACTIVATE 10
    #define TYPE_ACK 11
    #define TYPE_NACK 12

    #define MAX_MOLES     4
   
   
    enum messageTypes{
   DATA =1,
   DATA_PACKET_ACK = 2
};



#endif