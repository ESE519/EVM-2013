#include "packetHandler.h"
#include <include.h>
#include <ulib.h>
#include <stdio.h>




//To calculate the number of packets that have to be transmitted 
uint8_t calculateNumberPackets(int packetSize)
{
	uint8_t size=0;
	
	if(!packetSize)
	{
	size=0;
	}
	else
	{
	//Calculate the packet size	
	size = packetSize/MAX_PACKET_SIZE;
	 //If there is a remainder add one to the packet size 
		if( packetSize%MAX_PACKET_SIZE != 0)
			size++;
	}
	return size;
	
	
	
}


//create packet to be transmitted 

uint8_t createPacket (char *txBuffer, char *data)
{
	
	
	
	
	
	
	
	
	
	
	
}


