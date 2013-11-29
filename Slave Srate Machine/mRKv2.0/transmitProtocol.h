#include <include.h>
#include <ulib.h>
#include <stdio.h>

/************************************

Call these functions

***********************************/

/****************************************
 *
 *Checks if it is ready to send data to that
 *node
 *@params:node number
 *return: true or false
 *
 ****************************************/
int ReadyToSendData(uint8_t node);

/****************************************
 *Starts the transmission
 *@params:node number, number of bytes
 *return: void
 *
 ****************************************/
void startDataTransmission(int receiverNode,int numBytes);

/****************************************
 *Starts the transmission
 *@params:node number
 *return: number of bytes
 *
 ****************************************/

int checkReceivedDataReady(uint8_t node);
