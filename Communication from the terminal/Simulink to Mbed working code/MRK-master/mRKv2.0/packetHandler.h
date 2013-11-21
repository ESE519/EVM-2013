/*********************************************************
*
*
*
*
Header file to handle packet transmission and reception from one node to another
1) Each packet can have a maximum payload of 100 bytes 
2) will have other headers required to ensure the packet is received properly and transmitted properly 
3) Still thinking whether to ADD CRC to it . Professor wants it . 




*
*
*
*************************************************************/

#include <include.h>
#include <ulib.h>
#include <stdio.h>


#define MAX_PACKET_SIZE 100  //Defines the maximum size of the packet 
#define HEADER_SIZE 5

enum packetType{
	FIRST_PACKET=1,
	INTERMEDIATE_PACKET =2,
	LAST_PACKET=3
};

enum transmissionStates{
	START=1,
	WAIT_ACK =2,
	IN_PROGRESS = 3,
  LAST_PACKET_ACK =4,
	END=5
};	
enum receiverStates{
    SEND_ACK=1,
    STOP=2
};



uint8_t calculateNumberPackets(int packetSize);
int createNextTaskPacket(char *txBuffer,uint8_t *data,int dataSize);
void extractPacket(uint8_t *rxBuffer,uint8_t *data,int packetSize);