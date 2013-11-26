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
#include <ff_basic_sensor.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include "messageTypes.h"
#include "packetHandler.h"
#include "UtsavAPI.h"

//#define MASTER 1
//#define RECEIVER_NODE 2
// Only require MAC address for address decode
//#define MAC_ADDR    0x0001
// Change this to your group channel
#define MY_CHANNEL 2

#define MAX_SLAVES 4
#define MY_ID 2

Serial pc(USBTX,USBRX);

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type RE_TX_TASK;
NRK_STK re_tx_task_stack[NRK_APP_STACKSIZE];
void re_tx_task (void);

nrk_task_type ACK_TX_TASK;
NRK_STK ack_tx_task_stack[NRK_APP_STACKSIZE];
void ack_tx_task (void);

nrk_task_type SERIAL_READ_TASK;
NRK_STK serial_rx_task_stack[NRK_APP_STACKSIZE];
void serial_rx_task (void);

nrk_task_type RX_TASK2;
NRK_STK rx_task2_stack[NRK_APP_STACKSIZE];
void rx_task2 (void);


nrk_task_type NM_TASK;
NRK_STK nm_task_stack[NRK_APP_STACKSIZE];
void nm_task (void);

void nrk_create_taskset ();
void startDataTransmission(int receiverNode,int numBytes);
int checkReceivedDataReady();
int ReadyToSendData();
int getReceivedDataSize();


char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char ack_buf[5];

/*************************************************************

Global variables related to code transmission


*************************************************************/
uint8_t data[1024];                         //Buffer to store the data to be transmitted (Each node is given a buffer size of 512)
uint8_t receiveData[1024];								//Buffer to store the received data 
char sendNextPacket = END;											//Handles the state while sending
char receiverNextPacket = STOP;									//Handles the state while receiving
int sequenceNumber = 0;													//Sequence Number of the system
int lastSentSequenceNo[MAX_SLAVES] ={0};					//Record of the last sent sequence number
int lastReceivedSequenceNo[MAX_SLAVES]={0};			//Record of the last received sequence number
int lastPacketTransmitted=0;													
int transmittedPacketLength =0;									//Gives the length of the transmitted packet
int packetSize=1024;														//Max packet size	
char sendAckFlag=0;															//Flag to tell send the ACK 						
char receiveComplete =1 ;												//Flag to inform receiveComplete
int receivedPacketSize=0;												//Keeps an account of number bytes received 
int gindex=0;



int main(void)

{  printf("started\r\n");
	char code[10];
	for(int i=0;i<10;i++)
		code[i]=61;
	char funcname[20]="Simple_function\0";
    set_FuncCode(receiveData,1,20,code);
	printf("Reached\r\n");	
	/*
	   int r;
    
    
    for(r=0;r<1024;r++)
    {
        data[r] = r;

    }
		startDataTransmission(1,256);
    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
		
		nrk_start();
    return 0;
    */
}

void rx_task ()
{ 

    uint8_t rssi,len,*local_rx_buf,v;
    int receivedSeqNo=0,senderNode = 0;
    
		bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer (rx_buf,RF_MAX_PAYLOAD_SIZE);
    
    
    while(!bmac_started());
    printf("Receiver node Bmac initialised\n");

    while(1) {
        

        if( !bmac_rx_pkt_ready())
        continue;
        printf("received packet\n\r");
        nrk_led_toggle(ORANGE_LED);
        
				local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);

        //If my own packet then dont receive
        if(local_rx_buf[SOURCE_ADDRESS_LOCATION]==MY_ID)
        {
            continue;
        }
        //Sample the data
        receivedSeqNo = local_rx_buf[SEQUENCE_NUM_LOCATION];
        senderNode = local_rx_buf[SOURCE_ADDRESS_LOCATION];


        if(len!=0 && local_rx_buf[DESTINATION_ADDRESS_LOCATION]==MY_ID)
        {
            switch(local_rx_buf[MESSAGE_TYPE_LOCATION])
            {
            case TASK:
                if(receivedSeqNo>lastReceivedSequenceNo[senderNode])
                {
                    receiveComplete =0;
										printf("received new packet\n\r");
										//Process and send the acknowledgement to the receiver
                    lastReceivedSequenceNo[senderNode]=receivedSeqNo;
                    extractPacket(local_rx_buf,receiveData,len);
										if(local_rx_buf[PACKET_NUM_LOCATION]==FIRST_PACKET)
										{
											receivedPacketSize=0;
											receivedPacketSize +=(len-HEADER_SIZE); 
											
										}
										else
										{
											
											receivedPacketSize +=(len-HEADER_SIZE); 
											
										}
										
                    ack_buf[DESTINATION_ADDRESS_LOCATION]=senderNode;
                    ack_buf[SOURCE_ADDRESS_LOCATION]= MY_ID;
                    ack_buf[SEQUENCE_NUM_LOCATION] = receivedSeqNo;
                    ack_buf[MESSAGE_TYPE_LOCATION] = TASK_PACKET_ACK;
                    receiverNextPacket= SEND_ACK;
                    
										if(local_rx_buf[PACKET_NUM_LOCATION]==LAST_PACKET)
                    {
                        receiveComplete =1;
												
												for(int i=0;i<receivedPacketSize;i++)
                        {
                            printf("%d \t",receiveData[i]);

                        }
												
										}
										sendAckFlag=1;
                }
                else
                {
                    printf("received old packet\n\r");
										ack_buf[DESTINATION_ADDRESS_LOCATION]=senderNode;
                    ack_buf[SOURCE_ADDRESS_LOCATION]= MY_ID;
                    ack_buf[SEQUENCE_NUM_LOCATION] = lastReceivedSequenceNo[senderNode];
                    ack_buf[MESSAGE_TYPE_LOCATION] = TASK_PACKET_ACK;
                    //Send the acknowledgement to the sender
                    receiverNextPacket= SEND_ACK;
										sendAckFlag=1;

                }
                //v=nrk_event_signal( signal_ack );
								
                break;
            case TASK_PACKET_ACK:

                //If it is a new ACK packet then accept it. Else dont bother
                if(lastSentSequenceNo[senderNode]==receivedSeqNo)
                {
									printf("Received Ack\n");
										switch(sendNextPacket)
                    {
                    case WAIT_ACK:
                        sendNextPacket= IN_PROGRESS;
												printf("Sent IN_PROGRESS\n");
												break;
                    case LAST_PACKET_ACK:
												printf("Sent end_PROGRESS\n");
                        sendNextPacket = END;
                        break;
                    default:
                        printf("Error shouldn't have entered here");
                        break;

                    }
									
								}


                break;
								default:
									break;

            }
        }
        bmac_rx_pkt_release ();
				nrk_wait_until_next_period();
    }
    // pointing the function pointer to the copied code in the flash

}

void rx_task2 (void) {
	 uint8_t rssi,len,*local_rx_buf,v;
    int receivedSeqNo=0,senderNode = 0;
    
		bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer (rx_buf,RF_MAX_PAYLOAD_SIZE);
    
    
    while(!bmac_started());
    printf("Receiver node Bmac initialised\n");

    while(1) {
        

        if( !bmac_rx_pkt_ready())
					continue;
        printf("received packet\n\r");
        nrk_led_toggle(ORANGE_LED);
				
				if(checkReceivedDataReady())
					continue;
        
				local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);

        //If my own packet then dont receive
        if(local_rx_buf[SOURCE_ADDRESS_LOCATION]==MY_ID)
        {
            continue;
        }
        //Sample the data
        receivedSeqNo = local_rx_buf[SEQUENCE_NUM_LOCATION];
        senderNode = local_rx_buf[SOURCE_ADDRESS_LOCATION];
        char* slaves;

        if(len!=0 && local_rx_buf[DESTINATION_ADDRESS_LOCATION]==MY_ID )
        {
            switch(receiveData[DATA_TYPE])
            {
            case PING:
              checkPings(senderNode);
              break;
            case FUNC_NAMES:
                 
              break;
						case FUNC_CODE:
								
							break;
						case TASK_PARAMS:
							
							break;
						case INIT_STATES:
							
							break;
						case CHECKPOINT_STATES:
							
							break;
						case TASK_ACTIVATE:
							
							break;
						case TASK_DEACTIVATE:
							
							break;
						case ACK:
							
							break;
						case NACK:
							
							break;
						
						default:
							break;

            }
        }
        bmac_rx_pkt_release ();
				nrk_wait_until_next_period();
    }
	
}

/** Ping slaves task*/
void nm_task(){
	
	while(1){
		//printf("nm task scheduled \r\n");
		if(!ReadyToSendData())
			continue;
	startDataTransmission(MASTER_ID,1);
	set_MessageTypes(receiveData,PING);
		
	nrk_wait_until_next_period();
	}
}

void tx_task ()
{
    int r;
    uint8_t val;

    nrk_sig_t tx_done_signal;
    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);
    tx_buf[SOURCE_ADDRESS_LOCATION] = MY_ID;
    //tx_buf[DESTINATION_ADDRESS_LOCATION] = RECEIVER_NODE;
    printf("Transmit Task\r\n");
		while(1){

        switch(sendNextPacket)
        {
        case START:
            tx_buf[SEQUENCE_NUM_LOCATION] = ++sequenceNumber;
						
            lastSentSequenceNo[tx_buf[DESTINATION_ADDRESS_LOCATION]]= sequenceNumber;
            r=createNextTaskPacket(tx_buf,data,packetSize);
            transmittedPacketLength = r+HEADER_SIZE;
            printf("%d\n",transmittedPacketLength);    
						
							
								if(r == 100)
                {  
										nrk_led_toggle(RED_LED);
										sendNextPacket = WAIT_ACK;
								}
                else
                {
                   	//printf("Transmitted  lower number of Packet\r\n");
										sendNextPacket = LAST_PACKET_ACK;
										nrk_led_toggle(BLUE_LED);
								}
								val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
								printf("Transmit Task\r\n");
								if(val!=NRK_OK)
								{
								    printf("ERROR IN TRANSMISSION\r\n");
								}
            
            
            
            break;

        case IN_PROGRESS:
            tx_buf[SEQUENCE_NUM_LOCATION] = ++sequenceNumber;
						printf("Transmit Task\r\n");
            r=createNextTaskPacket(tx_buf,data,packetSize);
            lastSentSequenceNo[tx_buf[DESTINATION_ADDRESS_LOCATION]]= sequenceNumber;
            transmittedPacketLength = r+HEADER_SIZE;
                printf("%d\n",transmittedPacketLength);    
								if(r == 100)
                {
										nrk_led_toggle(RED_LED);
										sendNextPacket = WAIT_ACK;
                    								}
                else
                {
                   nrk_led_toggle(BLUE_LED);
										printf("last packet\n");
										sendNextPacket = LAST_PACKET_ACK;
								}
								val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
								printf("Transmit Task\r\n");
								if(val != NRK_OK)
								{
                printf("ERROR IN TRANSMISSION\r\n");
								}
            break;
        case WAIT_ACK:
        case END:
        default:
            break;



        }

        nrk_wait_until_next_period();
    }

}

void re_tx_task()
{

    uint8_t val;
    nrk_sig_t tx_done_signal;

    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);

    while(1)
    {

        switch(sendNextPacket)
        {
        case LAST_PACKET_ACK:
        case WAIT_ACK:
            val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
            if(val == NRK_OK)
            {
                printf("Retransmitted\r\n");
            }
            else
            {
                printf("ERROR IN  RETRANSMISSION\r\n");

            }
            break;
        default:
            break;
        }




        nrk_wait_until_next_period();



    }
}
void ack_tx_task()
{

    uint8_t val;
    nrk_sig_t tx_done_signal;

    while (!bmac_started ())
        nrk_wait_until_next_period ();
		//tx_done_signal = bmac_get_tx_done_signal ();
    
    
    nrk_signal_register (tx_done_signal);
    //printf("Ack task\n");
		while(1)
    {



        
        //printf("Signal Received\n");
				if(sendAckFlag)
        {
					sendAckFlag=0;
            switch(receiverNextPacket)
            {
            case SEND_ACK:
                val=bmac_tx_pkt(ack_buf,5);
								printf("Sent Ack\n");
                receiverNextPacket = STOP;
                break;
            default:
                break;
            }
        }
        nrk_wait_until_next_period();
    }

}


void serial_rx_task()
{
int readVal=0;
    while(1)
    {
				/*
        if(pc.readable())
        {
            scanf("%d",&readVal);
            
						nrk_led_toggle(RED_LED);
						
							if(sendNextPacket == END)
							{
								transmittedPacketLength=1024;	
								sendNextPacket = START;
							}
						
				}
				*/

			nrk_wait_until_next_period();
    }

}
/******************************************************************

API for receiving and sending data 

******************************************************************/
int ReadyToSendData()
{
	
	if(sendNextPacket==END)
	{
		//Returns 1 if it is ready to transmit 
		return 1;
	}
	else
	{
		//Return 0 if it is not ready to transmit
		return 0;
	}
}

//Initiates the task transmission
void startDataTransmission(int receiverNode,int numBytes)
{
	tx_buf[DESTINATION_ADDRESS_LOCATION]= receiverNode;
	packetSize=numBytes;
	sendNextPacket =START;
}
//Checks if the data is received . If complete returns the size of the data else returns 0
int checkReceivedDataReady()
{
			if(receiveComplete)
			{
				return receivedPacketSize;
			}	
		return 0;
}


void nrk_create_taskset ()
{


    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 10;
    RX_TASK.FirstActivation = TRUE;
    RX_TASK.Type = BASIC_TASK;
    RX_TASK.SchType = PREEMPTIVE;
    RX_TASK.period.secs = 500;
    RX_TASK.period.nano_secs = 0;
    RX_TASK.cpu_reserve.secs = 0;
    RX_TASK.cpu_reserve.nano_secs = 200 * NANOS_PER_MS;
    RX_TASK.offset.secs = 0;
    RX_TASK.offset.nano_secs = 0;
    

    RX_TASK2.task = rx_task2;
    nrk_task_set_stk( &RX_TASK2, rx_task2_stack, NRK_APP_STACKSIZE);
    RX_TASK2.prio = 10;
    RX_TASK2.FirstActivation = TRUE;
    RX_TASK2.Type = BASIC_TASK;
    RX_TASK2.SchType = PREEMPTIVE;
    RX_TASK2.period.secs = 5;
    RX_TASK2.period.nano_secs = 0;
    RX_TASK2.cpu_reserve.secs = 0;
    RX_TASK2.cpu_reserve.nano_secs = 300 * NANOS_PER_MS;
    RX_TASK2.offset.secs = 0;
    RX_TASK2.offset.nano_secs = 0;
    
		TX_TASK.task = tx_task;
    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
    TX_TASK.prio = 10;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = 0;
    TX_TASK.period.nano_secs = 500 * NANOS_PER_MS;
    TX_TASK.cpu_reserve.secs = 0;
    TX_TASK.cpu_reserve.nano_secs = 200 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    

    RE_TX_TASK.task = re_tx_task;
    nrk_task_set_stk( &RE_TX_TASK, re_tx_task_stack, NRK_APP_STACKSIZE);
    RE_TX_TASK.prio = 1;
    RE_TX_TASK.FirstActivation = TRUE;
    RE_TX_TASK.Type = BASIC_TASK;
    RE_TX_TASK.SchType = PREEMPTIVE;
    RE_TX_TASK.period.secs = 1;
    RE_TX_TASK.period.nano_secs = 0;
    RE_TX_TASK.cpu_reserve.secs = 0;
    RE_TX_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    RE_TX_TASK.offset.secs = 0;
    RE_TX_TASK.offset.nano_secs = 0;
    


    ACK_TX_TASK.task = ack_tx_task;
    nrk_task_set_stk( &ACK_TX_TASK, ack_tx_task_stack, NRK_APP_STACKSIZE);
    ACK_TX_TASK.prio = 1;
    ACK_TX_TASK.FirstActivation = TRUE;
    ACK_TX_TASK.Type = BASIC_TASK;
    ACK_TX_TASK.SchType = PREEMPTIVE;
    ACK_TX_TASK.period.secs = 1;
    ACK_TX_TASK.period.nano_secs = 0;
    ACK_TX_TASK.cpu_reserve.secs = 0;
    ACK_TX_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    ACK_TX_TASK.offset.secs = 0;
    ACK_TX_TASK.offset.nano_secs = 0;
		
	  
		NM_TASK.task = nm_task;
    nrk_task_set_stk( &NM_TASK, nm_task_stack, NRK_APP_STACKSIZE);
    NM_TASK.prio = 1;
    NM_TASK.FirstActivation = TRUE;
    NM_TASK.Type = BASIC_TASK;
    NM_TASK.SchType = PREEMPTIVE;
    NM_TASK.period.secs = 1;
    NM_TASK.period.nano_secs = 0;
    NM_TASK.cpu_reserve.secs = 0;
    NM_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    NM_TASK.offset.secs = 0;
    NM_TASK.offset.nano_secs = 0;
  

/*
    SERIAL_READ_TASK.task = serial_rx_task;
    nrk_task_set_stk( &SERIAL_READ_TASK, serial_rx_task_stack, NRK_APP_STACKSIZE);
    SERIAL_READ_TASK.prio = 1;
    SERIAL_READ_TASK.FirstActivation = TRUE;
    SERIAL_READ_TASK.Type = BASIC_TASK;
    SERIAL_READ_TASK.SchType = PREEMPTIVE;
    SERIAL_READ_TASK.period.secs = 5;
    SERIAL_READ_TASK.period.nano_secs = 0;
    SERIAL_READ_TASK.cpu_reserve.secs = 0;
    SERIAL_READ_TASK.cpu_reserve.nano_secs = 20 * NANOS_PER_MS;
    SERIAL_READ_TASK.offset.secs = 0;
    SERIAL_READ_TASK.offset.nano_secs = 0;
*/    
		nrk_activate_task (&RX_TASK);
		nrk_activate_task (&RX_TASK2);
		nrk_activate_task (&TX_TASK);
		nrk_activate_task (&RE_TX_TASK);
		nrk_activate_task (&ACK_TX_TASK);
		nrk_activate_task (&NM_TASK);
		//nrk_activate_task (&SERIAL_READ_TASK);
		
		
    printf ("Create done\r\n");
}
