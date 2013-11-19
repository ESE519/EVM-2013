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
	uint8_t rssi;
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

#define MAX_TTL								5
// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...

#define MY_ID 1
#define BROADCAST_ID 255
#define GATEWAY_ID	1

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



void sendACK_task (void);
//Neighbor message task
nrk_task_type SEND_ACK_TASK;
NRK_STK send_ack_task_stack[NRK_APP_STACKSIZE];




volatile uint8_t receivedResponse = 0;
//Sequence Number for all the packets received
uint32_t sequenceNumber=1;


//Arrays to store the slaves list and also routing table and last received sequence number



//Function Definitions
void nrk_create_taskset ();
void checkSlaves(char*);
void ReduceTTL();
void addtoRoutingtable();
void nrk_register_drivers();
int read_light_sensor();
int read_temp_sensor();
int route_to_Gateway(struct Message_Structure *);
void send_ack(uint8_t node_id);

int sensor_data_ack_received = 1;
int task_count = 10;
int send_ack_flag = 0;
uint8_t dest_send_ack;

struct RoutingTableEntry 		routing_table[10];


void updateRSSI(int id, uint8_t rssi) {
	routing_table[id].rssi = rssi;
}


void serialize(char *out_array, struct Message_Structure *msg){
	int i;
	out_array[0] = msg->source_id;
	out_array[1] = msg->dest_id;
	out_array[2] = msg->next_hop;
	memcpy(&out_array[3], &(msg->seq_no), 2);
	out_array[5] = msg->msg_type;
	memcpy(&out_array[6], msg->data, 50);
	
//	if(msg->msg_type == NEIGHBOR_MSG) {
//		printf("serialize:");
//		for(i = 0; i < 10; i++) {
//			printf("%d  ", msg->data[i]);
//		}
//	printf("\n\r");
//	}
	
	
}


void deserialize(struct Message_Structure *msg, char *out_array) {
	msg->source_id = out_array[0] ;
	msg->dest_id = out_array[1];
	msg->next_hop = out_array[2];
	memcpy(&(msg->seq_no), &out_array[3], 2);
	msg->msg_type = out_array[5];
	memcpy(msg->data, &out_array[6], 50);
	int i;
//	if(msg->msg_type == NEIGHBOR_MSG) {
//		printf("deserialize:");
//		for(i = 0; i < 10; i++) {
//			printf("%d  ", msg->data[i]);
//		}
//	printf("\n\r");
//	}
	
}


void initialize_routing_table() {
	int i;
	for(i = 0; i < MAX_SLAVES + 1; i++) {
		routing_table[i].dest_id = -1;
		routing_table[i].next_hop = -1;
		routing_table[i].cost = -1;
		routing_table[i].ttl = 0;
		routing_table[i].lastReceivedSeqNo = 0;
		routing_table[i].rssi = 0;
	}
	
	
}


int main(void)
{   
		nrk_setup_ports();
	  nrk_init();
		nrk_register_drivers();
    bmac_task_config();
		printf("afasdfasdfasdf\n\r");
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
		initialize_routing_table();
    nrk_start();
    return 0;

}


void print_on_console(struct Message_Structure *rx_msg)
{
	uint16_t light_data, temp_data;
	char data[128];
	int i, count;
	memcpy(&light_data, &(rx_msg->data[0]), 2);
	memcpy(&temp_data, &(rx_msg->data[2]), 2);
	printf("from %d\n\r", rx_msg->source_id);
	printf("Light: %d\tTemp:%d\n\r", light_data, temp_data);
	
	printf("N: ");
	count = 4;
	
	for(i = 1; i < MAX_SLAVES + 1; i++) {
		if(rx_msg->data[count++] != 0)
			printf("%d\t%d\n\r", i, rx_msg->data[count+MAX_SLAVES-1]);
	}
	
	dest_send_ack = rx_msg->source_id;
	send_ack_flag = 1;
}


void sendACK_task() {
	struct Message_Structure txMessage;
	
	uint8_t val;
	nrk_sig_t sd_done_signal; // Signal registering to get transmission done

	//Wait till BMAC is ready
	
	//Set the tx signal
	sd_done_signal = bmac_get_tx_done_signal ();
	nrk_signal_register (sd_done_signal);




	
		
		txMessage.source_id = MY_ID;
		txMessage.dest_id = 0;
		txMessage.next_hop = 0;
		txMessage.seq_no = sequenceNumber++;
		txMessage.msg_type = SENSOR_DATA_ACK;
		
		while(1) {
			nrk_wait_until_next_period();
			if(!send_ack_flag)
				continue;
			txMessage.dest_id = dest_send_ack;
			send_ack_flag = 0;
			route_to_Gateway(&txMessage);         // transmitting the packet
		}
	
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
			routing_table[i].lastReceivedSeqNo = 0;
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


		char tx_buf[10];

    while(1){
			//printf("sent slave disc\n\r");
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
		
		if(routing_table[i].cost < 7 && routing_table[i].cost > 0) {
			printf("dest: %d\tnext hop: %d\tCost: %d\tmax_ttl:%d \n\r", i, routing_table[i].next_hop,routing_table[i].cost, routing_table[i].ttl);
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
    
	  
    
		routing_table[recFrom].next_hop = recFrom;
		routing_table[recFrom].cost = 1;
		routing_table[recFrom].ttl = MAX_TTL;
		routing_table[recFrom].lastReceivedSeqNo = msg->seq_no;	
				
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
	char nv_buf[60];
	for(i = 1; i < MAX_SLAVES +1; i++) {
		if(routing_table[i].cost < 7 && routing_table[i].cost > 0)
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
        //dump_neighbors();
			  printf("\n");
        sendNeighborMsg();
        //dump_Routes();
        nrk_wait_until_next_period();

    }

}

//Processing the Neighbor Message Received

void processNeighborMsg(struct Message_Structure *msg){

	uint8_t receivedFrom = msg->source_id;
	uint8_t receivedCost, node_id, count = 0;
	int i;
	
	//if(msg->seq_no <= routing_table[msg->source_id].lastReceivedSeqNo)
	//	return;
	//printf("RC is : ");
	for(i = 1; i < MAX_SLAVES + 1; i++) {
		
		receivedCost = msg->data[count++];
		//printf("%d\t", receivedCost);
		if(i == MY_ID)
			continue;
		
		if(i == receivedFrom)
			continue;
		
		if(routing_table[i].next_hop == receivedFrom) {
			routing_table[i].cost = receivedCost + 1;
			routing_table[i].ttl = MAX_TTL;
		}
		
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
			routing_table[i].ttl = MAX_TTL;
			
		}
		
		
			
		
	}
	printf("\n\r");
	
}		



void rx_task ()
{
		char rx_buf[60];
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
						updateRSSI(rxMessage.source_id, rssi);
						
								printf("%d: %d  %d\n\r", rxMessage.source_id, rssi,rxMessage.msg_type);
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
						case SENSOR_DATA:
							if(rssi > RSSI_THRESHOLD)
							{
								printf("Sensor Data Received\r\n");
								if(rxMessage.dest_id == MY_ID)
									print_on_console(&rxMessage);
								
								else if(rxMessage.next_hop == MY_ID)
									route_to_Gateway(&rxMessage);
								
							}
							break;
							
						case SENSOR_DATA_ACK:
							if(rssi > RSSI_THRESHOLD)
							{
								if(rxMessage.dest_id == MY_ID) {
									sensor_data_ack_received = 1;
									task_count = 10;
									printf("received ack\n\r");
								}
								
								else if(rxMessage.next_hop == MY_ID)
									route_to_Gateway(&rxMessage);
								
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
    printf("breaks out\r\n");

	}

	
	
void tx_task ()
{
		struct Message_Structure txMessage;
	  int count = 0, i, r;
    nrk_sig_t tx_done_signal;
    uint8_t val, sequenceNumber;
	  uint16_t light_Value,temp_Value;
    nrk_time_t r_period;
	
	  
    while (!bmac_started ())
     nrk_wait_until_next_period ();
   
		printf("Sender node bmac initialised\r\n");
		
    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);
    while(1) {
			nrk_wait_until_next_period ();   
			if(MY_ID == GATEWAY_ID)
				continue;
			
			 if(task_count != 0)
				task_count--;
			 if(task_count == 0 || (sensor_data_ack_received == 0)) {
			
			 light_Value=read_light_sensor();
			 
			 temp_Value=read_temp_sensor();
			 
			 txMessage.source_id = MY_ID;
			 txMessage.dest_id = GATEWAY_ID;
			 txMessage.next_hop = 0;
			 txMessage.seq_no = sequenceNumber++;
			 txMessage.msg_type = SENSOR_DATA;
			 
			 memcpy(&txMessage.data[0], &light_Value, 2);
			 memcpy(&txMessage.data[2], &temp_Value, 2);
			 count = 4;
			
			 for(i = 1; i < MAX_SLAVES + 1; i++) {
				 
				 if(routing_table[i].cost == 1)
					 txMessage.data[count++] = i;
				 
				 else
					 txMessage.data[count++] = 0;
			 }
//			 for(i = 1; i < MAX_SLAVES + 1; i++)
//					txMessage.data[count++] = routing_table[i].rssi;
			 
			 r = route_to_Gateway(&txMessage);
			 sensor_data_ack_received = 0;
			 if(r != 0) {
				 printf("No route to gateway\n\r");
			 }
				 
			 nrk_led_toggle(BLUE_LED);
		 }
			 
    }
}



int read_light_sensor() {
    uint8_t buf[2],val;
    uint16_t adc_int;
    int fd;
    fd=nrk_open(FIREFLY_SENSOR_BASIC,READ);
    wait_ms(1);
    val = nrk_set_status(fd, SENSOR_SELECT, LIGHT);
    val = nrk_read(fd, &buf[0], 2);
    adc_int = buf[0] + (buf[1]<<8);
    return adc_int;
}

int read_temp_sensor() {
    uint8_t buf[2],val;
    uint16_t adc_int;
    int fd;
    fd=nrk_open(FIREFLY_SENSOR_BASIC,READ);
    wait_ms(1);
    val = nrk_set_status(fd, SENSOR_SELECT, TEMP);
    val = nrk_read(fd, &buf[0], 2);
    adc_int = buf[0] + (buf[1]<<8);
    return adc_int;
}


void nrk_register_drivers(){
    int8_t val;
    val=nrk_register_driver( &dev_manager_ff_sensors,FIREFLY_SENSOR_BASIC);
    if(val==NRK_ERROR) printf("Failed to load my ADC driver\r\n");
}


int route_to_Gateway(struct Message_Structure *msg){
	int i, val;
	char tx_buf[60];
	/*nrk_sig_t tx_done_signal;
	tx_done_signal = bmac_get_tx_done_signal ();
	*/
	uint8_t destinationNode = msg->dest_id;
   //nrk_signal_register (tx_done_signal);
	
	if(routing_table[destinationNode].next_hop != -1) {
		msg->next_hop = routing_table[destinationNode].next_hop;
		serialize(tx_buf, msg);
		val=bmac_tx_pkt((char*)tx_buf, 60);
	  printf("From %d to %d\r\n", msg->source_id, msg->dest_id);
		return 0;
	}
	
	else
		return -1;
	
}


void nrk_create_taskset ()
{
    nrk_status_t status;
    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 1;
    RX_TASK.FirstActivation = TRUE;
    RX_TASK.Type = BASIC_TASK;
    RX_TASK.SchType = PREEMPTIVE;
    RX_TASK.period.secs = 0;
    RX_TASK.period.nano_secs =250 * NANOS_PER_MS;
    RX_TASK.cpu_reserve.secs =0;
    RX_TASK.cpu_reserve.nano_secs = 150 * NANOS_PER_MS;
    RX_TASK.offset.secs = 0;
    RX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&RX_TASK);


    TX_TASK.task = tx_task;
    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
    TX_TASK.prio = 1;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = 3;
    TX_TASK.period.nano_secs = 0;
    TX_TASK.cpu_reserve.secs = 0;
    TX_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);

    SD_TASK.task = slaveDiscovery_task;
    nrk_task_set_stk( &SD_TASK, sd_task_stack, NRK_APP_STACKSIZE);
    SD_TASK.prio = 3;
    SD_TASK.FirstActivation = TRUE;
    SD_TASK.Type = BASIC_TASK;
    SD_TASK.SchType = PREEMPTIVE;
    SD_TASK.period.secs = 3;
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
    NM_TASK.period.secs = 3;
    NM_TASK.period.nano_secs = 0;
    NM_TASK.cpu_reserve.secs = 0;
    NM_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
    NM_TASK.offset.secs = 0;
    NM_TASK.offset.nano_secs = 0;
    status= nrk_activate_task (&NM_TASK);
		
		SEND_ACK_TASK.task = sendACK_task;
    nrk_task_set_stk(&SEND_ACK_TASK, send_ack_task_stack, NRK_APP_STACKSIZE);
    SEND_ACK_TASK.prio = 2;
    SEND_ACK_TASK.FirstActivation = TRUE;
    SEND_ACK_TASK.Type = BASIC_TASK;
    SEND_ACK_TASK.SchType = PREEMPTIVE;
    SEND_ACK_TASK.period.secs = 0;
    SEND_ACK_TASK.period.nano_secs = 700 * NANOS_PER_MS;
    SEND_ACK_TASK.cpu_reserve.secs = 0;
    SEND_ACK_TASK.cpu_reserve.nano_secs = 10 * NANOS_PER_MS;
    SEND_ACK_TASK.offset.secs = 0;
    SEND_ACK_TASK.offset.nano_secs = 0;
    status= nrk_activate_task (&SEND_ACK_TASK);

   
}

