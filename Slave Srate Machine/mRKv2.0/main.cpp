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
#include "transmitProtocol.h"
#include "slaveStates.h"
#include "UtsavAPI.h"
#include "function_manager.h"
#include "memory_manager.h"
#include "state_manager.h"

// Only require MAC address for address decode
//#define MAC_ADDR    0x0001



// Change this to your group channel
#define MY_CHANNEL 2

//My ID 
#define MY_ID 2

//Reset the pointer to the correct buffers associated with the node number
#define TX_LOCATION(ADD) ((ADD-1)*RF_MAX_PAYLOAD_SIZE)
#define DATA_LOCATION(ADD) ((ADD-1)*512)
#define MASTER_NODE 1

//Change this according to the utilization of the node (only for communication)
#define COMM_UTILIZATION		0.5733
#define NUM_VIRTUAL_TASKS		3

Serial pc(USBTX,USBRX);





//Receive task
nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

//Transmit Task
nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

//Retransmission Task
nrk_task_type RE_TX_TASK;
NRK_STK re_tx_task_stack[NRK_APP_STACKSIZE];
void re_tx_task (void);

//Acknowledgement Task
nrk_task_type ACK_TX_TASK;
NRK_STK ack_tx_task_stack[NRK_APP_STACKSIZE];
void ack_tx_task (void);

//Slave task
nrk_task_type SLAVE_TASK;
NRK_STK slave_task_stack[NRK_APP_STACKSIZE];
void slave_task (void);

/************ VIRTUAL TASKS **************************************/
nrk_task_type VIRTUAL_TASKS[NUM_VIRTUAL_TASKS];
NRK_STK virtual_tasks_stack[NUM_VIRTUAL_TASKS][NRK_APP_STACKSIZE];
void virtual_task_0();
void virtual_task_1();
void virtual_task_2();
/******************************************************************/


/********************* EVM FUNCTIONS ***************************/
void evm_init();
int is_schedulable(uint16_t period, uint16_t wcet);
/**************************************************************/


/******************* EVM Global Variables **********************/
uint8_t enable_virtual_tasks[NUM_VIRTUAL_TASKS];
uint32_t code_aligned[512];
uint16_t func_reply;
/**************************************************************/

//Creates the task sets required 
void nrk_create_taskset ();


/*************** Private functions ****************/
void send_ack_signal();
void send_nack_signal();
void send_ack_func_names(uint16_t reply);

/***************************************************

Transmit , receive and acknowledgement buffers


****************************************************/
char tx_buf[RF_MAX_PAYLOAD_SIZE*MAX_MOLES];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char ack_buf[MAX_MOLES+1][5]={0};



/*************************************************************

Global variables related to code transmission


*************************************************************/
uint8_t data[512*MAX_MOLES];                             //Buffer to store the data to be transmitted
uint8_t receiveData[512*MAX_MOLES];						 					//Buffer to store the received data
char sendNextPacket[MAX_MOLES+1] ;					 						//Handles the state while sending
char receiverNextPacket[MAX_MOLES+1] ;			 						//Handles the state while receiving
int sequenceNumber= 0;                                  //Sequence Number of the system
int lastSentSequenceNo[MAX_MOLES+1] ={0};					 			//Record of the last sent sequence number
int lastReceivedSequenceNo[MAX_MOLES+1]={0};			     	//Record of the last received sequence number
int transmittedPacketLength[MAX_MOLES+1] ={0};				 	//Gives the length of the transmitted packet
int packetSize[MAX_MOLES+1]={0};							 					//Max packet size
char sendAckFlag[MAX_MOLES+1]={0};							 				//Flag to tell send the ACK
char receiveComplete[MAX_MOLES+1];					 						//Flag to inform receiveComplete
int receivedPacketSize[MAX_MOLES+1]={0};					 			//Keeps an account of number bytes received
int lastPacketRead[MAX_MOLES+1]={0};
/*********************************************

Intializes the global variables 

***************************************************/
void init ()
{
    int i=0;
    memset(sendNextPacket,END,MAX_MOLES+1);					 		//Initialize transmit state machine

    memset(receiverNextPacket,STOP,MAX_MOLES+1); 			 //Initialize the receive state machine

    memset(receiveComplete,1,MAX_MOLES+1);					 	//Initiaze the receive complete state machine
}



void evm_init() {
	int r;
	function_manager_init();
	r = set_start_address(0xE000);
	if(r) 
		printf("start address setting failed\n\r");

	r = set_end_address(0xF000);
	if(r)
		printf("end address setting failed\n\r");
	
	
	function_register("get_state", strlen("get_state") + 1, (char *)&get_state);
	function_register("checkpoint_state", strlen("checkpoint_state") + 1, (char *)&checkpoint_state);
}




int is_schedulable(uint16_t period_ms, uint16_t wcet_ms) {
	int i;
	float u;
	float communication_utilization;
	//const float communication_utilization = ((float)RX_TASK_RES_MS + RX_TASK_RES_S*1000)/((float)RX_TASK_PERIOD_MS + RX_TASK_PERIOD_S*1000)      + 
	//																			((float)TX_TASK_RES_MS + TX_TASK_RES_S*1000)/((float)TX_TASK_PERIOD_MS + TX_TASK_PERIOD_S*1000); 
	
	communication_utilization = COMM_UTILIZATION;

	u = communication_utilization;
	
	for(i = 0; i < NUM_VIRTUAL_TASKS; i++) {
		if(enable_virtual_tasks[i]) {
			u += (VIRTUAL_TASKS[i].cpu_reserve.nano_secs / NANOS_PER_MS * 1000000 + VIRTUAL_TASKS[i].cpu_reserve.secs) / (VIRTUAL_TASKS[i].period.nano_secs / NANOS_PER_MS * 1000000 + VIRTUAL_TASKS[i].period.secs);
	  }
	}
	
	u += (float)wcet_ms/period_ms;
	//printf("if added u is %.2f\n\r", u);
	if(u < 0.69f)
		return 1;
	else
		return 0;
}







/*********************** virtual functions ***************************/
void virtual_task_0() {
	return;
}

void virtual_task_1() {
	return;
}
void virtual_task_2() {
	return;
}
void virtual_task_3() {
	return;
}
/***************************************************************/


int test_ref() {
	int a, b;
	a = 1;
	b = 3;
	return a+b;
}


int main(void)

{
    int r;
    int i;
    
    // Data to be transmitted
    for(i=0;i<MAX_MOLES;i++)
    {
        for(r=0;r<512;r++)
        {
            data[r+(512*i)] = r;

        }
    }
    // Initialize the packet handlers
    init();
    initPacketHandler();
		evm_init();
		//function_register("test_ref", strlen("test_ref") + 1, (char *)&test_ref);
    //Start transmission
    //startDataTransmission(2,10);
    //startDataTransmission(1,10);

    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    nrk_start();
    
    return 0;

}
/******************************************************************************************************
 *
 *
 *Receive task execution
 *
 *
 *
 *
 ******************************************************************************************************/
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
        //printf("received packet\n\r");
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

        //Check if the message is intended for me
        if(len!=0 && local_rx_buf[DESTINATION_ADDRESS_LOCATION]==MY_ID)
        {
            //Check the type of data
            switch(local_rx_buf[MESSAGE_TYPE_LOCATION])
            {
            case DATA:
							if(lastPacketRead[senderNode]==0)
							{
                if(receivedSeqNo>lastReceivedSequenceNo[senderNode])
                {
                    receiveComplete[senderNode] =0;
                    printf("received new packet\n\r");
                    //Process and send the acknowledgement to the receiver
                    lastReceivedSequenceNo[senderNode]=receivedSeqNo;
                    extractPacket(local_rx_buf,&receiveData[DATA_LOCATION(senderNode)],len,senderNode);
                    if(local_rx_buf[PACKET_NUM_LOCATION]==FIRST_PACKET)
                    {
                        receivedPacketSize[senderNode]=0;
                        receivedPacketSize[senderNode] +=(len-HEADER_SIZE);

                    }
                    else
                    {

                        receivedPacketSize[senderNode] +=(len-HEADER_SIZE);

                    }

                    //Send the acknowledgement
                    ack_buf[senderNode][DESTINATION_ADDRESS_LOCATION]=senderNode;
                    ack_buf[senderNode][SOURCE_ADDRESS_LOCATION]= MY_ID;
                    ack_buf[senderNode][SEQUENCE_NUM_LOCATION] = receivedSeqNo;
                    ack_buf[senderNode][MESSAGE_TYPE_LOCATION] = DATA_PACKET_ACK;
                    receiverNextPacket[senderNode]= SEND_ACK;
                    
                    if(local_rx_buf[PACKET_NUM_LOCATION]==LAST_PACKET)
                    {
                        receiveComplete[senderNode] =1;
												lastPacketRead[senderNode]=1;
                        for(int i=0;i<receivedPacketSize[senderNode];i++)
                        {
                           // printf("%d \t",receiveData[i]);

                        }

                    }
                    sendAckFlag[senderNode]=1;
                }
                else
                {
                    printf("received old packet\n\r");
                    ack_buf[senderNode][DESTINATION_ADDRESS_LOCATION]=senderNode;
                    ack_buf[senderNode][SOURCE_ADDRESS_LOCATION]= MY_ID;
                    ack_buf[senderNode][SEQUENCE_NUM_LOCATION] = lastReceivedSequenceNo[senderNode];
                    ack_buf[senderNode][MESSAGE_TYPE_LOCATION] = DATA_PACKET_ACK;
                    //Send the acknowledgement to the sender
                    receiverNextPacket[senderNode]= SEND_ACK;
                    sendAckFlag[senderNode]=1;

                }
                //v=nrk_event_signal( signal_ack );
							}
                break;
            case DATA_PACKET_ACK:

                //If it is a new ACK packet then accept it. Else dont bother
                if(lastSentSequenceNo[senderNode]==receivedSeqNo)
                {
                    printf("Received Ack\n");
                    switch(sendNextPacket[senderNode])
                    {
                    case WAIT_ACK:
                        sendNextPacket[senderNode]= IN_PROGRESS;
                        //printf("Sent IN_PROGRESS\n");
                        break;
                    case LAST_PACKET_ACK:
                        //printf("Sent end_PROGRESS\n");
                        sendNextPacket[senderNode] = END;
												break;
                    default:
                        //printf("Error shouldn't have entered here");
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
/***************************************************************
 *
 *
 *Transmit task
 *
 *
 *
 ***************************************************************/


void tx_task ()
{
    int r;
    int i=0,j=0;
    uint8_t val;
    nrk_sig_t tx_done_signal;
    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);
    for(i=1;i<=MAX_MOLES;i++)
    {
        tx_buf[SOURCE_ADDRESS_LOCATION + TX_LOCATION(i)] = MY_ID;
    }
    printf("Transmit Task\r\n");
    while(1){


        for(i=1;i<=MAX_MOLES;i++)
        {

            if(i==MY_ID)
            {
                continue;
            }
            //Check the state of the state machine
            switch(sendNextPacket[i])
            {
            //Start the transmission
            case START:
                tx_buf[SEQUENCE_NUM_LOCATION+TX_LOCATION(i)] = ++sequenceNumber;

                lastSentSequenceNo[i]= sequenceNumber;
                //Create a new packet
                r=createNextTaskPacket(&tx_buf[TX_LOCATION(i)],&data[DATA_LOCATION(i)],packetSize[i],i);
                //Decide the packet length
                transmittedPacketLength[i] = r+HEADER_SIZE;
                
								for(j=0;j<transmittedPacketLength[i];j++)
								//printf("%d \t",tx_buf[TX_LOCATION(i)+j]);
								
								if(r == 100)
                {
                    sendNextPacket[i] = WAIT_ACK;
                }
                else
                {
                    sendNextPacket[i] = LAST_PACKET_ACK;
                }
                val=bmac_tx_pkt(&tx_buf[TX_LOCATION(i)], transmittedPacketLength[i]);
                break;

            case IN_PROGRESS:
                tx_buf[SEQUENCE_NUM_LOCATION+TX_LOCATION(i)] = ++sequenceNumber;
                lastSentSequenceNo[i]= sequenceNumber;
                //Create a new packet
                r=createNextTaskPacket(&tx_buf[TX_LOCATION(i)],&data[DATA_LOCATION(i)],packetSize[i],i);
                //Decide the packet length
                transmittedPacketLength[i] = r+HEADER_SIZE;

                if(r == 100)
                {
                    sendNextPacket[i] = WAIT_ACK;
                }
                else
                {
                    printf("last packet\n");
                    sendNextPacket[i] = LAST_PACKET_ACK;
                }
                val=bmac_tx_pkt(&tx_buf[TX_LOCATION(i)], transmittedPacketLength[i]);
                break;
            case WAIT_ACK:
            case END:
            default:
                break;



            }
        }

        nrk_wait_until_next_period();
    }

}
/***************************************************************************
 *
 *
 *
 *Retransmission Task
 *
 *
 *
 ****************************************************************************/
void re_tx_task()
{

    int i=0;
    uint8_t val;
    nrk_sig_t tx_done_signal;

    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);
    //Retransmission
    while(1)
    {
        for(i=1;i<=MAX_MOLES;i++)
        {

            if(i==MY_ID)
            {
                continue;
            }
            switch(sendNextPacket[i])
            {
            case LAST_PACKET_ACK:
            case WAIT_ACK:
                val=bmac_tx_pkt(&tx_buf[TX_LOCATION(i)], transmittedPacketLength[i]);
                printf("Retransmitted\r\n");
                break;
            default:
                break;
            }



        }
        nrk_wait_until_next_period();



    }
}
/**********************************************************************************************
 *
 *
 *
 *Acknowledgement task
 *
 *
 *
 ***********************************************************************************************/

void ack_tx_task()
{
    int i=0;
    uint8_t val;
    nrk_sig_t tx_done_signal;

    while (!bmac_started ())
        nrk_wait_until_next_period ();
    //tx_done_signal = bmac_get_tx_done_signal ();
    
    
    nrk_signal_register (tx_done_signal);
    //printf("Ack task\n");
    while(1)
    {


        for(i=1;i<=MAX_MOLES;i++)
        {

            if(i==MY_ID)
            {
                continue;
            }

            //send Ack
            if(sendAckFlag[i])
            {
                sendAckFlag[i]=0;
                switch(receiverNextPacket[i])
                {
                case SEND_ACK:
                    val=bmac_tx_pkt(&(ack_buf[i][0]),5);
                    printf("Sent Ack\n");
                    receiverNextPacket[i] = STOP;
                    break;
                default:
                    break;

                }


            }
        }
        nrk_wait_until_next_period();



    }


}

void slave_task(){
	Signals slaveSignal;
	printf("Slave task\r\n");
	while(1){
		if(checkReceivedDataReady(MASTER_NODE)){
			
			if(checkPacketRead(MASTER_NODE) ) {
				
				slaveSignal = slave_sm(receiveData[DATA_LOCATION(MASTER_NODE)]);
				setPacketRead(MASTER_NODE);
				switch(slaveSignal) {
					case ACK_SIGNAL:
					send_ack_signal();
					break;
					
					case NACK_SIGNAL:
					send_nack_signal();
					break;
					
					case ACK_FUNC_NAMES:
					send_ack_func_names(func_reply);
					break;
					
					default:
					break;
				}
			}
		}
	}
}
/******************************************************************




API for receiving and sending data 



******************************************************************/
int ReadyToSendData(uint8_t node)
{

    if(sendNextPacket[node]==END)
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

    tx_buf[DESTINATION_ADDRESS_LOCATION+TX_LOCATION(receiverNode)]= receiverNode;
    packetSize[receiverNode] = numBytes;
    sendNextPacket[receiverNode] =START;
}
//Checks if the data is received . If complete returns the size of the data else returns 0

int checkReceivedDataReady(uint8_t node )
{
    if(receiveComplete[node])
    {
        return receivedPacketSize[node];
    }
    return 0;
}
//Checks if that packet has been read or not 
//1-New Packet ,,,,0-Old Packet
int checkPacketRead(uint8_t node)
{

return lastPacketRead[node];	
	
}
//Set that you read the old packet 
void setPacketRead(uint8_t node)
{
	
	lastPacketRead[node] = 0;
	
}
//Create all the tasks
void nrk_create_taskset ()
{


    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 3;
    RX_TASK.FirstActivation = TRUE;
    RX_TASK.Type = BASIC_TASK;
    RX_TASK.SchType = PREEMPTIVE;
    RX_TASK.period.secs = 1;
    RX_TASK.period.nano_secs =0;
    RX_TASK.cpu_reserve.secs = 0;
    RX_TASK.cpu_reserve.nano_secs = 200 * NANOS_PER_MS;
    RX_TASK.offset.secs = 0;
    RX_TASK.offset.nano_secs = 0;
    

    TX_TASK.task = tx_task;
    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
    TX_TASK.prio = 3;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = 1;
    TX_TASK.period.nano_secs = 0;
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
    RE_TX_TASK.period.secs = 3;
    RE_TX_TASK.period.nano_secs = 0;
    RE_TX_TASK.cpu_reserve.secs = 0;
    RE_TX_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    RE_TX_TASK.offset.secs = 0;
    RE_TX_TASK.offset.nano_secs = 0;
    


    ACK_TX_TASK.task = ack_tx_task;
    nrk_task_set_stk( &ACK_TX_TASK, ack_tx_task_stack, NRK_APP_STACKSIZE);
    ACK_TX_TASK.prio = 2;
    ACK_TX_TASK.FirstActivation = TRUE;
    ACK_TX_TASK.Type = BASIC_TASK;
    ACK_TX_TASK.SchType = PREEMPTIVE;
    ACK_TX_TASK.period.secs = 1;
    ACK_TX_TASK.period.nano_secs = 0;
    ACK_TX_TASK.cpu_reserve.secs = 0;
    ACK_TX_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    ACK_TX_TASK.offset.secs = 0;
    ACK_TX_TASK.offset.nano_secs = 0;

    
		SLAVE_TASK.task = slave_task;
    nrk_task_set_stk( &SLAVE_TASK, slave_task_stack, NRK_APP_STACKSIZE);
    SLAVE_TASK.prio = 9;
    SLAVE_TASK.FirstActivation = TRUE;
    SLAVE_TASK.Type = BASIC_TASK;
    SLAVE_TASK.SchType = PREEMPTIVE;
    SLAVE_TASK.period.secs = 5;
    SLAVE_TASK.period.nano_secs = 0;
    SLAVE_TASK.cpu_reserve.secs = 0;
    SLAVE_TASK.cpu_reserve.nano_secs = 200 * NANOS_PER_MS;
    SLAVE_TASK.offset.secs = 0;
    SLAVE_TASK.offset.nano_secs = 0;
    
		nrk_activate_task (&RX_TASK);
    nrk_activate_task (&TX_TASK);
    nrk_activate_task (&RE_TX_TASK);
    nrk_activate_task (&ACK_TX_TASK);
    nrk_activate_task (&SLAVE_TASK);
		
		
			VIRTUAL_TASKS[0].task = virtual_task_0;
    nrk_task_set_stk( &VIRTUAL_TASKS[0], virtual_tasks_stack[0], NRK_APP_STACKSIZE);
    VIRTUAL_TASKS[0].prio = 1;
    VIRTUAL_TASKS[0].FirstActivation = TRUE;
    VIRTUAL_TASKS[0].Type = BASIC_TASK;
    VIRTUAL_TASKS[0].SchType = PREEMPTIVE;
    VIRTUAL_TASKS[0].period.secs = 10;
    VIRTUAL_TASKS[0].period.nano_secs = 0;
    VIRTUAL_TASKS[0].cpu_reserve.secs = 0;
    VIRTUAL_TASKS[0].cpu_reserve.nano_secs = 0;
    VIRTUAL_TASKS[0].offset.secs = 0;
    VIRTUAL_TASKS[0].offset.nano_secs = 0;
	
		
		VIRTUAL_TASKS[1].task = virtual_task_1;
    nrk_task_set_stk( &VIRTUAL_TASKS[1], virtual_tasks_stack[1], NRK_APP_STACKSIZE);
    VIRTUAL_TASKS[1].prio = 1;
    VIRTUAL_TASKS[1].FirstActivation = TRUE;
    VIRTUAL_TASKS[1].Type = BASIC_TASK;
    VIRTUAL_TASKS[1].SchType = PREEMPTIVE;
    VIRTUAL_TASKS[1].period.secs = 10;
    VIRTUAL_TASKS[1].period.nano_secs = 0;
    VIRTUAL_TASKS[1].cpu_reserve.secs = 0;
    VIRTUAL_TASKS[1].cpu_reserve.nano_secs = 0;
    VIRTUAL_TASKS[1].offset.secs = 0;
    VIRTUAL_TASKS[1].offset.nano_secs = 0;
		
		
		VIRTUAL_TASKS[2].task = virtual_task_2;
    nrk_task_set_stk( &VIRTUAL_TASKS[2], virtual_tasks_stack[2], NRK_APP_STACKSIZE);
    VIRTUAL_TASKS[2].prio = 1;
    VIRTUAL_TASKS[2].FirstActivation = TRUE;
    VIRTUAL_TASKS[2].Type = BASIC_TASK;
    VIRTUAL_TASKS[2].SchType = PREEMPTIVE;
    VIRTUAL_TASKS[2].period.secs = 10;
    VIRTUAL_TASKS[2].period.nano_secs = 0;
    VIRTUAL_TASKS[2].cpu_reserve.secs = 0;
    VIRTUAL_TASKS[2].cpu_reserve.nano_secs = 0;
    VIRTUAL_TASKS[2].offset.secs = 0;
    VIRTUAL_TASKS[2].offset.nano_secs = 0;


    printf ("Create done\r\n");
}





void send_ack_signal() {
	set_MessageTypes(&data[DATA_LOCATION(MASTER_NODE)], TYPE_ACK);
	
	if(!ReadyToSendData(MASTER_NODE))
		printf("Error ready to send returned 0\n\r");
	
	startDataTransmission(MASTER_NODE, 10);
	printf("ACK signal sent\r\n");
}






void send_nack_signal() {
	set_MessageTypes(&data[DATA_LOCATION(MASTER_NODE)], TYPE_NACK);
	
	if(!ReadyToSendData(MASTER_NODE))
		printf("Error ready to send returned 0\n\r");
	
	startDataTransmission(MASTER_NODE, 10);
	printf("NACK signal sent\r\n");
}




void send_ack_func_names(uint16_t reply) {
	set_MessageTypes(&data[DATA_LOCATION(MASTER_NODE)], TYPE_ACK_NAMES);
	set_FuncReply(&data[DATA_LOCATION(MASTER_NODE)], reply);
	
	if(!ReadyToSendData(MASTER_NODE))
		printf("Error ready to send returned 0\n\r");
	
	startDataTransmission(MASTER_NODE, 10);
	printf("NACK signal sent\r\n");
}
