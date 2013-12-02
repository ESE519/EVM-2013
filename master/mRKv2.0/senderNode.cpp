
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

#include "function_manager.h"
#include "messagetypes.h"
#include "memory_manager.h"
#include "state_manager.h"

#include "states.h"

#include "packetHandler.h"
#include "transmitProtocol.h"
#include "UtsavAPI.h"

#define TX_LOCATION(ADD) ((ADD-1)*RF_MAX_PAYLOAD_SIZE)
#define DATA_LOCATION(ADD) ((ADD-1)*512)


// Change this to your group channel
#define MY_CHANNEL 2


#define MAX_RETRANSMISSION        10


// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...
#define MY_ID 1

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

//
nrk_task_type TOP_LEVEL_STATE_MACHINE_TASK;
NRK_STK top_level_sm_task_stack[NRK_APP_STACKSIZE];
void top_level_sm_task (void);


void nrk_create_taskset ();

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










/************* Variables for SM ******************/
TOP_LEVEL_SIGNAL my_signal = NO_SIGNAL_TOP;
uint16_t func_reply;
/****************************************/

/********** Task function ***********/
int simple_function(int task_num);
/*************************************/


/********* other functions ***************/
void print_function(char *ptr, int num);
/*****************************************/
extern char slavesList[MAX_SLAVES];

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



int simple_function_setup() {
	int ret_val, i;
	uint32_t init_states[10];
	ret_val = task_function_register("simple_function", strlen("simple_function") + 1, (char *)&simple_function);
	if(ret_val)
		return 1;
	
	/*ret_val = register_reference("simple_function", "print", strlen("print") + 1);
	if(ret_val)
		return 2;
	
	ret_val = register_reference("simple_function", "toggle", strlen("toggle") + 1);
	if(ret_val)
		return 3;*/
	
	/*ret_val = register_reference("simple_function", "test_ref", strlen("test_ref") + 1);
	if(ret_val)
		return 3;*/
	
	ret_val = register_reference("simple_function", "test_ref2", strlen("test_ref2") + 1);
	if(ret_val)
		return 3;
	
	ret_val = set_scheduling_parameters("simple_function", 1, 0, 0, 100);
	if(ret_val)
		return 4;
	
	for(i = 0; i < 10; i++)
		init_states[i] = i + 100;
	
	ret_val = set_task_states("simple_function", init_states);
	if(ret_val)
		return 5;
	
	return 0;
}



int test_ref() {
	int a, b;
	a = 1;
	b = 3;
	return a+b;	
}


int test_ref2() {

	
	int a, b, c;
	a = 1;
	b = 3;
	c = a*b + b;
	return c;
}

void print_function(char *ptr, int num) {
	for(; num > 0; num--) {
		printf("%x  ", *ptr);
		ptr++;
	}
	printf("\n\r");
}


int main(void)

{
		int r;
		uint32_t a;
		
	  evm_init();
	  r = simple_function_setup();
	  function_register("test_ref", 10, (char *)&test_ref);
		function_register("test_ref2", 14, (char *)&test_ref2);
		if(r) printf("Error: return val is %d\n\r",r);
	

	
	  init();
    initPacketHandler();
	
    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
	  
	  printf("just before task create\n\r");
    nrk_create_taskset();
		printf("just after task create\n\r");
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
	
    nrk_start();
	
    return 0;

}





int simple_function(int task_num){
	int a = 10, b = 2;
	uint16_t addrh, addrl;
	uint32_t address_to_get_handle;
	
	uint32_t task_state;
	int ret_val;
	
	void *(*get_function_handle)(const char *, int);
	int8_t (*led_toggle)(int);
	int (*print)(const char *, ...);	
	int (*get_state2)(int, uint8_t pos, uint32_t *ptr);
	int (*checkpoint_state2)(int, uint8_t pos, uint32_t val);
	int (*test_ref2_fn)();
	
	addrh = GET_HANDLE_ADDRESS_H;
	addrl = GET_HANDLE_ADDRESS_L;
	address_to_get_handle = *((uint32_t *)((addrh << 16) | addrl));
	
	// initialize get function handle function
	get_function_handle = (void *(*)(const char *, int)) address_to_get_handle;
	
	
	
	/************ Get the handles to all the functions ********************************/
	led_toggle = (int8_t (*)(int)) get_function_handle("toggle", 7);
	if(led_toggle == NULL)
		return 23;
	
	print = (int (*)(const char *, ...)) get_function_handle("print", 7);
	if(print == NULL)
		return 1;
	
	get_state2 = (int (*)(int, uint8_t, uint32_t *)) get_function_handle("get_state", 12);
	if(get_state == NULL)
		return 2;
	
	checkpoint_state2 = (int (*)(int, uint8_t, uint32_t)) get_function_handle("checkpoint_state", 18);
	if(checkpoint_state == NULL)
		return 3;
	
	test_ref2_fn = (int (*)()) get_function_handle("test_ref2", 11);
	if(test_ref2_fn == NULL)
		return 4;
	/*************************************************************************************/
	
	
	//get the current state
	ret_val = get_state2(task_num, 0, &task_state);
	if(ret_val != 0)
		return ret_val;
	
	
		
	led_toggle(RED_LED);
	print("current state: %d", task_state);
	
	task_state++;
	//checkpoint the state
	ret_val = checkpoint_state2(task_num, 0, task_state);
	if(ret_val != 0)
		return ret_val;
	
	ret_val = test_ref2_fn();
	return ret_val;
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
												lastPacketRead[senderNode] = 1;
                        /*for(int i=0;i<receivedPacketSize[senderNode];i++)
                        {
                            printf("%d \t",receiveData[i]);

                        }*/

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
                        printf("Sent IN_PROGRESS\n");
                        break;
                    case LAST_PACKET_ACK:
                        printf("Sent end_PROGRESS\n");
                        sendNextPacket[senderNode] = END;
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
                
								/*for(j=0;j<transmittedPacketLength[i];j++)
								printf("%d \t",tx_buf[TX_LOCATION(i)+j]);*/
								
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
       
       
   
   
void top_level_sm_task () {
	TOP_LEVEL_STATE my_state = STATE_START;
	my_signal = FOUND_NEIGHBORS;
	slavesList[0] = 10;
	
	printf("in top level task\n\n\n\r\r\r");
	while(1) {
		nrk_wait_until_next_period();
		
		nrk_led_toggle(BLUE_LED);
		if( checkReceivedDataReady(2)  ) {         //check if there is a new packet
			printf("entered inside first if\n\r");
			if(checkPacketRead(2)) {
				printf("received new signal: ");
				setPacketRead(2);             //Mark the packet as read.
				switch(receiveData[DATA_LOCATION(2)]) {
					case TYPE_ACK:
					printf("ACK\n\r");
					send_low_level_signal(ACK);
					break;
					
					
					case TYPE_NACK:
					printf("NACK: \n\r");
					send_low_level_signal(NACK);
					break;
					
					case TYPE_ACK_NAMES:
					memcpy(&func_reply, &receiveData[DATA_LOCATION(2) + 1], 2);
					printf("ACK FUNCTION NAMES: 0x%X\n\r", func_reply);
					send_low_level_signal(ACK);
					break;
					
					
					default:
					printf("Received unknown packet \n\r");
					break;
				}
			}
		}
		
		
		switch(my_state) {
			case STATE_START:
			switch(my_signal) {
				case FOUND_NEIGHBORS:
					
					send_middle_level_signal(FIND_NODE_SIGNAL);
					my_state = ASSIGN_TASKS;
					my_signal = NO_SIGNAL_TOP;
					break;
				
				default:
					my_state = STATE_START;
					my_signal = NO_SIGNAL_TOP;
			}
			break;
			
			case ASSIGN_TASKS:
				printf("Assign Tasks\n\r");
				switch(my_signal) {
					case ASSGN_DONE:
						my_state = STEADY_STATE;
						my_signal = NO_SIGNAL_TOP;
						break;
						
				}
			break;
				
			case STEADY_STATE:
				printf("Steady State\n\r");
				switch(my_signal) {
					case NODE_DIED:
						send_middle_level_signal(FIND_NODE_SIGNAL);
						my_state = ASSIGN_TASKS;
						my_signal = NO_SIGNAL_TOP;
						break;
				}
			break;
		}
		
		
		middle_level_take_action();
		low_level_take_action();
		
		
	}
}
				


void send_top_level_signal(TOP_LEVEL_SIGNAL sig) {
	my_signal = sig;
}
						
   
 



void nrk_create_taskset ()
{
    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 10;
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
    TX_TASK.prio = 10;
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
    RE_TX_TASK.period.secs = 4;
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
		
	  TOP_LEVEL_STATE_MACHINE_TASK.task = top_level_sm_task;
    nrk_task_set_stk( &TOP_LEVEL_STATE_MACHINE_TASK, top_level_sm_task_stack, NRK_APP_STACKSIZE);
    TOP_LEVEL_STATE_MACHINE_TASK.prio = 11;
    TOP_LEVEL_STATE_MACHINE_TASK.FirstActivation = TRUE;
    TOP_LEVEL_STATE_MACHINE_TASK.Type = BASIC_TASK;
    TOP_LEVEL_STATE_MACHINE_TASK.SchType = PREEMPTIVE;
    TOP_LEVEL_STATE_MACHINE_TASK.period.secs = 6;
    TOP_LEVEL_STATE_MACHINE_TASK.period.nano_secs = 0;
    TOP_LEVEL_STATE_MACHINE_TASK.cpu_reserve.secs = 0;
    TOP_LEVEL_STATE_MACHINE_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
    TOP_LEVEL_STATE_MACHINE_TASK.offset.secs = 0;
    TOP_LEVEL_STATE_MACHINE_TASK.offset.nano_secs = 0;
    
		
		
		nrk_activate_task (&RX_TASK);
    nrk_activate_task (&TX_TASK);
    nrk_activate_task (&RE_TX_TASK);
    nrk_activate_task (&ACK_TX_TASK);
		nrk_activate_task (&TOP_LEVEL_STATE_MACHINE_TASK);
		
		


    printf ("Create done\r\n");
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
    packetSize[receiverNode]=numBytes;
    sendNextPacket[receiverNode] =START;
}
//Checks if the data is received . If complete returns the size of the data else returns 0

int checkReceivedDataReady(uint8_t node)
{
    if(receiveComplete[node])
    {
        return receivedPacketSize[node];
    }
    return 0;
}
//Check that you have to read a new packet 
//new packet returns 1. Old packet - 0
int checkPacketRead(uint8_t node)
{
 return lastPacketRead[node];
		
}
//Set that you read the last packet 
void setPacketRead(uint8_t node)
{
	
	lastPacketRead[node] =0;
	
}