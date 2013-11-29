#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H


/*********************************************************
*
*
*
*
Header file to handle packet transmission and reception from one node to another
1) Each packet can have a maximum payload of 100 bytes 
2) will have other headers required to ensure the packet is received properly and transmitted properly 





*
*
*
*************************************************************/

#include <include.h>
#include <ulib.h>
#include <stdio.h>


#define MAX_PACKET_SIZE 100  //Defines the maximum size of the packet 
#define HEADER_SIZE 5				 //Defines the header size of the packet	


//Defines the different packet numbers required during transmission
enum packetType{
	FIRST_PACKET=1,
	INTERMEDIATE_PACKET =2,
	LAST_PACKET=3
};

//Defines the states associated with transmission of data 
enum transmissionStates{
	START=1,
	WAIT_ACK =2,
	IN_PROGRESS = 3,
  LAST_PACKET_ACK =4,
	END=5
};	

//Defines the states associated with receiving data 
enum receiverStates{
    SEND_ACK=1,
    STOP=2
};


/**********************************************************
Calculates the number of packets based on the size of data to
be transmitted 
@params: Size of the data to be transmitted
@return: The number of packets that need to be transmitted
************************************************************/
uint8_t calculateNumberPackets(int packetSize);


/**********************************************************
Creates the next packet to be transmitted 
@params: Transmission buffer , data , size of data and Receiver Node
@return: size of the packet created
************************************************************/
int createNextTaskPacket(char *txBuffer,uint8_t *data,int dataSize,int senderNode);

/**********************************************************
Extracts the packet and store in receive buffer  
@params: Receive buffer, data buffer and packet size
return: void
************************************************************/
void extractPacket(uint8_t *rxBuffer,uint8_t *data,int packetSize,int senderNode);

/**********************************************************
Initializes the packet handler
@params: 
return: 
************************************************************/
void initPacketHandler();



#endif