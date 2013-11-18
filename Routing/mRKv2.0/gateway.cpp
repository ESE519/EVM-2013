#include "mbed.h"
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
#include <nrk_time.h>
#include "basic_rf.h"
#include "bmac.h"
#include <ff_basic_sensor.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include "IAP.h"
#include "jumptable.h"
#include "messageTypes.h"

struct RoutingTableEntry {
	int dest_id;
	int next_hop;
	int cost;
	int ttl;
	uint16_t lastReceivedSeqNo;
};


struct Message_Structure {
	uint8_t source_id;
	uint8_t dest_id;
	uint8_t next_hop;
	uint16_t seq_no;
	uint8_t msg_type;
	char data[50];
};

// Change this to your group channel
#define MY_CHANNEL 2
//Number of retransmissions when a packet is not received.
#define MAX_RETRANSMISSION        10

#define MAX_TTL								3
// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...

#define MY_ID 3
#define BROADCAST_ID 255

#define MAX_SLAVES 4

#define RSSI_THRESHOLD    200

/*********************************Task declarations***************/

//Receive task to receive all data that is coming to a node

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];

void rx_task (void);
//Transmit Data required to be routed from one node to another
nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];

void tx_task (void);
//Slave Discovery Task to detect all the slaves of a node
nrk_task_type SD_TASK;
NRK_STK sd_task_stack[NRK_APP_STACKSIZE];

void slaveDiscovery_task (void);
//Neighbor message task
nrk_task_type NM_TASK;
NRK_STK nm_task_stack[NRK_APP_STACKSIZE];

void sendNeighborMsg_task(void);
//Buufers used for transmission and reception of data
char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char nv_buf[RF_MAX_PAYLOAD_SIZE];
char ni_buf[RF_MAX_PAYLOAD_SIZE];

volatile uint8_t receivedResponse = 0;
//Sequence Number for all the packets received
uint32_t sequenceNumber=1;


//Arrays to store the slaves list and also routing table and last received sequence number
char slavesList[MAX_SLAVES+1]={0};
char routingTable[MAX_SLAVES+1][MAX_SLAVES+1]={0};
uint16_t lightSensorData[MAX_SLAVES];
uint16_t tempSensorData[MAX_SLAVES];
uint32_t lastReceivedSequence[MAX_SLAVES+1];
char neighborTable[MAX_SLAVES][MAX_SLAVES];

//Function Definitions
void nrk_create_taskset ();
void checkSlaves(char*);
void ReduceTTL();
void addtoRoutingtable();



struct RoutingTableEntry 		routing_table[10];



void serialize(char *out_array, struct Message_Structure *msg){
	out_array[0] = msg->source_id;
	out_array[1] = msg->dest_id;
	out_array[2] = msg->next_hop;
	memcpy(&out_array[3], &(msg->seq_no), 2);
	out_array[5] = msg->msg_type;
	memcpy(&out_array[6], msg->data, 50);
}


void deserialize(struct Message_Structure *msg, char *out_array) {
	msg->source_id = out_array[0] ;
	msg->dest_id = out_array[1];
	msg->next_hop = out_array[2];
	memcpy(&(msg->seq_no), &out_array[3], 2);
	msg->msg_type = out_array[5];
	memcpy(msg->data, &out_array[6], 50);
}


void initialize_routing_table() {
	int i;
	for(i = 0; i < MAX_SLAVES + 1; i++) {
		routing_table[i].dest_id = -1;
		routing_table[i].next_hop = -1;
		routing_table[i].cost = -1;
		routing_table[i].ttl = 0;
		routing_table[i].lastReceivedSeqNo = 0;
	}
	
	
}


int main(void)
{   
		nrk_setup_ports();
	  nrk_init();
    bmac_task_config();
	printf("afasdfasdfasdf\n\r");
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
		initialize_routing_table();
    nrk_start();
    return 0;

}


void save_Neighbor_data(uint8_t *rx_buf)
{
	 //saving light data recieved from nodes
	 lightSensorData[rx_buf[0]]=rx_buf[5];// higher 8 bits of light value
 	 lightSensorData[rx_buf[0]]<<=8;
	 lightSensorData[rx_buf[0]]|=rx_buf[6]; //lower 8 bits of light value
   
   //saving tempersture data recieved from nodes	
	 tempSensorData[rx_buf[0]]=rx_buf[7];// higher 8 bits of temp vcalue
 	 tempSensorData[rx_buf[0]]<<=8;
	 tempSensorData[rx_buf[0]]|=rx_buf[8];//lower 8 bits of temp value
	
	 //saving neighbor information about all nodes
	 for(int i=8;i<MAX_SLAVES;i++){
		  if(rx_buf[i]!=0)
				neighborTable[rx_buf[0]][9-i]=1;
	}
  printf("Light sensor values received are: \r\n");
	for(int l=0;l<MAX_SLAVES;l++)
		printf("%d ",lightSensorData[l]);

}


//Check if the neighbor is alive or dead

void checkIfReceivedResponse()
{
	int i;
	for(i = 1; i < MAX_SLAVES + 1; i++) {
		if(i == MY_ID)
			continue;
		
		if(routing_table[i].ttl != 0) 
			routing_table[i].ttl--;
		
		if(routing_table[i].ttl == 0) {
			routing_table[i].next_hop = -1;
			routing_table[i].cost = -1;
		}
	}
}


//Task to send slave discovery messages to all its neighbors

void slaveDiscovery_task(){
		struct Message_Structure txMessage;
    uint8_t val;
    nrk_sig_t sd_done_signal; // Signal registering to get transmission done

    //Wait till BMAC is ready
    while (!bmac_started ())
        nrk_wait_until_next_period ();
    //Set the tx signal
    sd_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (sd_done_signal);




    while(1){
			printf("sent slave disc\n\r");
			txMessage.source_id = MY_ID;
			txMessage.dest_id = BROADCAST_ID;
			txMessage.next_hop = 0;
			txMessage.seq_no = sequenceNumber++;
			txMessage.msg_type = SLAVE_DISCOVERY;
			
			serialize(tx_buf, &txMessage);
			
			val=bmac_tx_pkt(tx_buf,7);          // transmitting the packet
			
			checkIfReceivedResponse();
	
			nrk_wait_until_next_period ();
    }
}

//Prints all the routes to all the nodes in the network

void dump_Routes()
{
	int i;
	printf("Dumping Routes of %d\n\r", MY_ID);
	for(i = 1; i < MAX_SLAVES + 1; i++) {
		if(i == MY_ID)
			continue;
		
		if(routing_table[i].cost != -1) {
			printf("dest: %d\tnext hop: %d\tCost: %d \n\r", i, routing_table[i].next_hop,routing_table[i].cost);
		}
	}
}

void dump_neighbors(){
	int i;
	printf("Neighbors of %d are: ",MY_ID); 
	for(i = 1; i < MAX_SLAVES + 1; i++) {
		if(i == MY_ID)
			continue;
		
		if(routing_table[i].cost == 1)
			printf("%d  ", i);
	}
	printf("\r\n");
}






//Processing the Neighbor Message Received

void processNeighborMsg(struct Message_Structure *msg){

	uint8_t receivedFrom = msg->source_id;
	uint8_t receivedCost, node_id, count = 0;
	int i;
	
	if(msg->seq_no <= routing_table[msg->source_id].lastReceivedSeqNo)
		return;
	
	for(i = 1; i < MAX_SLAVES + 1; i++) {
		receivedCost = msg->data[count++];
		if(i == MY_ID)
			continue;
		
		if(i == receivedFrom)
			continue;
		
		if(routing_table[i].next_hop == receivedFrom)
			routing_table[i].cost = receivedCost + 1;
		
		if(routing_table[i].cost == -1) {
				if(receivedCost < 7) {
					routing_table[i].cost = 1 + receivedCost;
					routing_table[i].next_hop = receivedFrom;
					routing_table[i].ttl = MAX_TTL;
				}		
		}
		
		
		else if( (int)(1 + receivedCost) < (routing_table[i].cost)) {
			routing_table[i].next_hop = receivedFrom;
			routing_table[i].cost = 1 + receivedCost;
			
		}
		
		if(routing_table[i].cost != -1)
			routing_table[i].ttl = MAX_TTL;
		
	}
	
}		
		
	

//			int flag=0;
//    uint8_t val;    //Value to hold transmit Success or failure
//    uint8_t destinationNode=0,receivedCost=0,receivedFrom=0;
//    uint32_t recSequence=0;
//    receivedFrom = rxBuf[0];                //Node from which the packet was received
//    recSequence = rxBuf[2];                 //Sequence Number of the received packet

//    if(recSequence>lastReceivedSequence[receivedFrom]){                    //checking for duplicates
//        lastReceivedSequence[receivedFrom] = recSequence;
//        for(int k=3; k < length;k++){
//							flag=0;
//            destinationNode = (rxBuf[k]&0xF0)>>4;        //getting the first 4 bits as destination ID
//            receivedCost =     rxBuf[k] & 0x0F;     //getting the last 4 bits as cost
//            if(destinationNode == MY_ID)    // ignore if the destination address is my
//						continue;
//						
//						if(destinationNode == 0) 
//						continue;
//						
//						for(int l=1; l <= MAX_SLAVES;l++){
//                if(routingTable[destinationNode][l]!=0){
//										flag=1;		
//										if((1+receivedCost)< routingTable[destinationNode][l]){
//											// a low cost route discovered
//											routingTable[destinationNode][l] = 0;
//											routingTable[destinationNode][receivedFrom] = (1 + receivedCost);
//											//printf("%d Edited a route to %d through %d at cost %d\r\n",MY_ID,destinationNode,receivedFrom,1+receivedCost);
//											break;
//                }
//            }
//        }
//				//Checking if there is no route to that destination node
//				if(flag==0)
//			   {
//					routingTable[destinationNode][receivedFrom] = (1 + receivedCost);
//					//printf("%d Added a route to %d through %d at cost %d\r\n",MY_ID,destinationNode,receivedFrom,1+receivedCost);
//				 }
//        }
//    }


	

//Adding to Neighbor table

void addToNeighbor(struct Message_Structure *msg)

{
    uint8_t i;
    uint8_t recFrom=0;
    uint16_t recSequence=0;
    recFrom = msg->source_id;
    
	  if(recFrom==0)
			printf("caught error");
    if(msg->seq_no > routing_table[recFrom].lastReceivedSeqNo)
    {
				routing_table[recFrom].next_hop = recFrom;
				routing_table[recFrom].cost = 1;
				routing_table[recFrom].ttl = MAX_TTL;
				routing_table[recFrom].lastReceivedSeqNo = msg->seq_no;	
    }
}



void sendNeighborMsg(){
	int i;
	uint8_t count = 0;
	int val;
	struct Message_Structure nMessage;
	nMessage.source_id = MY_ID;
	nMessage.dest_id = BROADCAST_ID;
	nMessage.next_hop = 0;
	nMessage.seq_no = sequenceNumber++;
	nMessage.msg_type = NEIGHBOR_MSG;
	
	for(i = 1; i < MAX_SLAVES +1; i++) {
		if(routing_table[i].cost != -1)
			nMessage.data[count++] = (uint8_t)routing_table[i].cost;
		else
			nMessage.data[count++] = 100;
	}
	
	serialize(nv_buf, &nMessage);			
	val=bmac_tx_pkt(nv_buf,60);//Transmit the routing table to each neighbor.
}


void sendNeighborMsg_task(){
		 while (!bmac_started ())
					nrk_wait_until_next_period ();
		 
    while(1){

        //printf("neighbor message task scheduled\r\n");
        dump_neighbors();
			  printf("\n");
        sendNeighborMsg();
        dump_Routes();
        nrk_wait_until_next_period();

    }

}





void rx_task ()
{
		struct Message_Structure rxMessage;
    uint8_t rssi,len,*local_rx_buf, from, received_round;
    int i,r,fromMole;
    bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer (rx_buf,RF_MAX_PAYLOAD_SIZE);
    int val;
    while(!bmac_started());
    printf("Receiver node Bmac initialised\n");
    while(1) {
        nrk_wait_until_next_period();
        if( !bmac_rx_pkt_ready())
            continue;
        nrk_led_toggle(ORANGE_LED);
        local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);
        //from=local_rx_buf[0];
        // printf("RSSI: %d\n\r",rssi);
        if(len!=0)
        {
						deserialize(&rxMessage, (char *)local_rx_buf);
            if(rxMessage.source_id != MY_ID) {
						
						switch(rxMessage.msg_type)
						{
						case SLAVE_DISCOVERY:
								if(rssi > RSSI_THRESHOLD)
								{
										addToNeighbor(&rxMessage);
								}
								break;
						case NEIGHBOR_MSG:
								//    printf("Received Neighbor Message");
								if(rssi > RSSI_THRESHOLD)
								{
										processNeighborMsg(&rxMessage);
								}
								break;
						case 3:
							if(rssi > RSSI_THRESHOLD)
							{
								save_Neighbor_data(local_rx_buf);
							}
							break;
						default:
								//Do nothing as of now
								break;
						}
					}
        }
        bmac_rx_pkt_release ();
    }


	}

void tx_task ()

{

    nrk_sig_t tx_done_signal;
    uint8_t val, sequenceNumber;
     nrk_time_t r_period;
    while (!bmac_started ())
    nrk_wait_until_next_period ();
    printf("Sender node bmac initialised\r\n");
    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);
		
    while(1) {        /*
            if(!received_response){
            tx_buf[0] = MY_ID;
            tx_buf[1] = 256; //function Size
            nrk_led_toggle(ORANGE_LED);
            val=bmac_tx_pkt(tx_buf, 102);
            //printf("Transmitted packet");
            nrk_wait_until_next_period ();
        }              */
    }
}



void nrk_create_taskset ()
{
    nrk_status_t status;
    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 2;
    RX_TASK.FirstActivation = TRUE;
    RX_TASK.Type = BASIC_TASK;
    RX_TASK.SchType = PREEMPTIVE;
    RX_TASK.period.secs = 0;
    RX_TASK.period.nano_secs = 50 * NANOS_PER_MS;
    RX_TASK.cpu_reserve.secs = 0;
    RX_TASK.cpu_reserve.nano_secs = 10 * NANOS_PER_MS;
    RX_TASK.offset.secs = 0;
    RX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&RX_TASK);


//    TX_TASK.task = tx_task;
//    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
//    TX_TASK.prio = 1;
//    TX_TASK.FirstActivation = TRUE;
//    TX_TASK.Type = BASIC_TASK;
//    TX_TASK.SchType = PREEMPTIVE;
//    TX_TASK.period.secs = 3;
//    TX_TASK.period.nano_secs = 0;
//    TX_TASK.cpu_reserve.secs = 0;
//    TX_TASK.cpu_reserve.nano_secs = 600 * NANOS_PER_MS;
//    TX_TASK.offset.secs = 0;
//    TX_TASK.offset.nano_secs = 0;
//    nrk_activate_task (&TX_TASK);

    SD_TASK.task = slaveDiscovery_task;
    nrk_task_set_stk( &SD_TASK, sd_task_stack, NRK_APP_STACKSIZE);
    SD_TASK.prio = 3;
    SD_TASK.FirstActivation = TRUE;
    SD_TASK.Type = BASIC_TASK;
    SD_TASK.SchType = PREEMPTIVE;
    SD_TASK.period.secs = 5;
    SD_TASK.period.nano_secs = 0;
    SD_TASK.cpu_reserve.secs = 0;
    SD_TASK.cpu_reserve.nano_secs = 50 * NANOS_PER_MS;
    SD_TASK.offset.secs = 0;
    SD_TASK.offset.nano_secs = 0;
    nrk_activate_task (&SD_TASK);

    NM_TASK.task = sendNeighborMsg_task;
    nrk_task_set_stk(&NM_TASK, nm_task_stack, NRK_APP_STACKSIZE);
    NM_TASK.prio = 2;
    NM_TASK.FirstActivation = TRUE;
    NM_TASK.Type = BASIC_TASK;
    NM_TASK.SchType = PREEMPTIVE;
    NM_TASK.period.secs = 10;
    NM_TASK.period.nano_secs = 0;
    NM_TASK.cpu_reserve.secs = 0;
    NM_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    NM_TASK.offset.secs = 0;
    NM_TASK.offset.nano_secs = 0;
    status= nrk_activate_task (&NM_TASK);

   
}

