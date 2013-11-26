#include "packetHandler.h"
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include "messageTypes.h"



/**********************
Global Definitions 

**********************/




//Global variables to handle transmission
uint8_t firstPacket[MAX_MOLES+1];					//Define first packet 
int numberOfPackets[MAX_MOLES+1] = {0};				//Defines the number of packets 
int packetNumber[MAX_MOLES+1];						//Defines the current packet number 

//Global variables to handle reception 
int packetNumReceived[MAX_MOLES+1] ={0};

//To calculate the number of packets that have to be transmitted 
uint8_t calculateNumberPackets(int packetSize)
{
	uint8_t size=0;
	
	if(packetSize==0)
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
	printf("Packets : %d\n",size);
	return size;
	
	
	
}


//create packet to be transmitted 
int createNextTaskPacket(char *txBuffer,uint8_t *data,int dataSize,int senderNode)
{
	int returnVal;
	if(firstPacket[senderNode]==1)
	{
	//Get the number of packets that are required 	
	numberOfPackets[senderNode]=calculateNumberPackets(dataSize);
	firstPacket[senderNode] = 0;
	
	}
	printf("Data first bit is %d   %d \n",data[((packetNumber[senderNode])-1)*MAX_PACKET_SIZE],(packetNumber[senderNode]-1));
	//Create a packet 
	txBuffer[MESSAGE_TYPE_LOCATION] = DATA;
	
	
	
	
	if(packetNumber[senderNode] == 1 &&packetNumber[senderNode] != numberOfPackets[senderNode])
	{
		txBuffer[PACKET_NUM_LOCATION] = FIRST_PACKET;//First packet 	
		
		memcpy(&txBuffer[DATA_START_LOCATION],&data[(packetNumber[senderNode]-1)*MAX_PACKET_SIZE],MAX_PACKET_SIZE);//Copy data into the transmit buffer
    returnVal = MAX_PACKET_SIZE;
	}
	
	else if(packetNumber[senderNode] == numberOfPackets[senderNode])
	{
		txBuffer[PACKET_NUM_LOCATION] = LAST_PACKET;	//Last packet when packet number is same as number of packets
	
		memcpy(&txBuffer[DATA_START_LOCATION],&data[(packetNumber[senderNode]-1)*MAX_PACKET_SIZE],(dataSize%MAX_PACKET_SIZE)); ////Copy data into the transmit buffer with right size
		firstPacket[senderNode]=1;
		packetNumber[senderNode] = 0; 
    returnVal =(dataSize%MAX_PACKET_SIZE);
	}
	else
	{
		txBuffer[PACKET_NUM_LOCATION] = INTERMEDIATE_PACKET;  // All other intermediate packets 
		memcpy(&txBuffer[DATA_START_LOCATION],(char *)&data[(packetNumber[senderNode]-1)*MAX_PACKET_SIZE],MAX_PACKET_SIZE); //Copy data into the transmit buffer 
    returnVal = MAX_PACKET_SIZE;
	}
    printf("Transmitted Packet No %d",packetNumber[senderNode]);
	//Increment the packet number
	(packetNumber[senderNode])++;
	return returnVal;
}
//extract the packet data packet from the receive buffer 
void extractPacket(uint8_t *rxBuffer,uint8_t *data,int packetSize,int senderNode)
{
	
	memcpy (&data[packetNumReceived[senderNode]*MAX_PACKET_SIZE],&rxBuffer[HEADER_SIZE],packetSize-HEADER_SIZE);
	(packetNumReceived[senderNode])++;
    printf("Received Packet No %d",packetNumReceived[senderNode]);
    if(rxBuffer[PACKET_NUM_LOCATION] == LAST_PACKET)
	{
		packetNumReceived[senderNode] = 0;
	}

}

void initPacketHandler()
{
	int i;
	
		for(i=0;i<=MAX_MOLES;i++)
	{
		packetNumber[i] =1;
		firstPacket[i]=1;
		printf("%d \t %d \t",packetNumber[i],firstPacket[i]);
		}
}