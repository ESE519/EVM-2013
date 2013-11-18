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



// Change this to your group channel
#define MY_CHANNEL 4
//Number of retransmissions when a packet is not received.
#define MAX_RETRANSMISSION        10


// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...

#define MY_ID 3
#define GATEWAY_ID 1
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


//Dont know why this is used as of now

volatile uint8_t receivedResponse = 0;


//Sequence Number for all the packets received

uint32_t sequenceNumber=1;


//Arrays to store the slaves list and also routing table and last received sequence number

char slavesList[MAX_SLAVES+1]={0};
char routingTable[MAX_SLAVES+1][MAX_SLAVES+1]={0};
uint32_t lastReceivedSequence[MAX_SLAVES+1];



//Function Definitions
void nrk_register_drivers();
void nrk_create_taskset ();
void checkSlaves(char*);
void addtoRoutingtable();
int read_light_sensor();
int read_temp_sensor();
void route_to_Gateway(uint8_t*);

int main(void)
{   
		nrk_setup_ports();
	  nrk_init();
    nrk_register_drivers();
	  bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    nrk_start();
    return 0;

}




//Check if the neighbor is alive or dead

void checkIfReceivedResponse()

{

    char i;
    for(i=1;i<=MAX_SLAVES;i++)
    {
        if(i!=MY_ID && slavesList[i]!=0)
        {
            slavesList[i]--;
            if(slavesList[i]==0)
            {
               //printf("Removed node %d",i);
                //removing from routing table
                for(int k=1 ;k < MAX_SLAVES+1;k++)
                {
                    //The first case cannot happen cause you are trying to kill all routes to the that destination
                    routingTable[i][k]=0;  //if the removed node was a destination node in the entry
                    routingTable[k][i]=0;  //if the removed node was a next hop in the entry
                }
            }
        }
    }
}


//Task to send slave discovery messages to all its neighbors

void slaveDiscovery_task(){
    uint8_t val;
    nrk_sig_t sd_done_signal; // Signal registering to get transmission done

    //Wait till BMAC is ready
    while (!bmac_started ())
        nrk_wait_until_next_period ();
    //Set the tx signal
    sd_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (sd_done_signal);




    while(1){
        tx_buf[0] = MY_ID;                  // indicates packet from master
        tx_buf[1] = SLAVE_DISCOVERY;        // slave discovery message
        tx_buf[2] = sequenceNumber;         // sequenceNumber of the packet
        sequenceNumber++;                   // incrementing the sequence Number
        val=bmac_tx_pkt(tx_buf,3);          // transmitting the packet
        checkIfReceivedResponse();
        nrk_wait_until_next_period ();
    }
}


//Prints all the routes to all the nodes in the network

void dump_Routes()

{
    for(int i=1; i<=MAX_SLAVES;i++)
    {
        for(int k=1;k<=MAX_SLAVES;k++)
        {
            if(routingTable[i][k]!=0)
            {
                printf("Found route to %d via %d aat cost of %d\r\n",i,k,routingTable[i][k]);
            }
        }
    }
}

void dump_neighbors(){
	printf("Neighbors of %d are: ",MY_ID); 
	for(int i=1;i<=MAX_SLAVES;i++){
		if(slavesList[i]!=0)
			printf("%d ",i);
	}
	printf("\r\n");
}






//Processing the Neighbor Message Received

void processNeighborMsg(uint8_t *rxBuf, int length){


			int flag=0;
    uint8_t val;    //Value to hold transmit Success or failure
    uint8_t destinationNode=0,receivedCost=0,receivedFrom=0;
    uint32_t recSequence=0;
    receivedFrom = rxBuf[0];                //Node from which the packet was received
    recSequence = rxBuf[2];                 //Sequence Number of the received packet

    if(recSequence>lastReceivedSequence[receivedFrom]){                    //checking for duplicates
        lastReceivedSequence[receivedFrom] = recSequence;
        for(int k=3; k < length;k++){
							flag=0;
            destinationNode = (rxBuf[k]&0xF0)>>4;        //getting the first 4 bits as destination ID
            receivedCost =     rxBuf[k] & 0x0F;     //getting the last 4 bits as cost
            if(destinationNode == MY_ID)    // ignore if the destination address is my
						continue;
						
						if(destinationNode == 0) 
						continue;
						
						for(int l=1; l <= MAX_SLAVES;l++){
                if(routingTable[destinationNode][l]!=0){
										flag=1;		
										if((1+receivedCost)< routingTable[destinationNode][l]){
											// a low cost route discovered
											routingTable[destinationNode][l] = 0;
											routingTable[destinationNode][receivedFrom] = (1 + receivedCost);
											//printf("%d Edited a route to %d through %d at cost %d\r\n",MY_ID,destinationNode,receivedFrom,1+receivedCost);
											break;
                }
            }
        }
				//Checking if there is no route to that destination node
				if(flag==0)
			   {
					routingTable[destinationNode][receivedFrom] = (1 + receivedCost);
					//printf("%d Added a route to %d through %d at cost %d\r\n",MY_ID,destinationNode,receivedFrom,1+receivedCost);
				 }
        }
    }
}


//Adding to Neighbor table

void addToNeighbor(uint8_t *rxBuf)
{
    uint8_t i;
    uint8_t recFrom=0;
    uint32_t recSequence=0;
    recFrom = rxBuf[0];
    recSequence = rxBuf[2];
    if(recSequence>lastReceivedSequence[recFrom] && slavesList[recFrom]==0)
    {
        if(recFrom != MY_ID){
					slavesList[recFrom]=5;                             //10 is set to ensure that when 10s timeout happens remove the node
					lastReceivedSequence[recFrom] = recSequence;        //Set the last received sequence number as the new sequence number
					routingTable[recFrom][recFrom]=1;
				}
    }
    /*    printf("Neighbors of %d :",MY_ID);
    for(i=1;i<=MAX_SLAVES;i++)
    {
        if(slavesList[i]!=0)
        {
            printf("\t%d\t",i);
        }
    }
*/

    uint8_t nextHop=0, cost=0, destID=0;
}


void sendNeighborMsg(){
    uint8_t val;
    char index=0;
    nv_buf[0] = MY_ID;
    nv_buf[1] = NEIGHBOR_MSG;
    nv_buf[2] =sequenceNumber;
    sequenceNumber++;
    for(int i=1;i<=MAX_SLAVES;i++){
        for (int k=1;k<=MAX_SLAVES;k++){
            //*first 4 bits are destination and last 4 bits are cost.
            if (routingTable[i][k]!=0){ //if an entry exists fro a destination node
                index++;
                nv_buf[2+index] = i;    //destination node
                nv_buf[2+index]=nv_buf[2+index]<<4; //first bits as destination
                nv_buf[2+index] |= routingTable[i][k] ;  //last 4 bits as cost
            }
        }
    }
    val=bmac_tx_pkt(nv_buf,index+3);//Transmit the routing table to each neighbor.
}


void sendNeighborMsg_task(){
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
    uint8_t rssi,len,*local_rx_buf,mole, from, received_round;
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
        from=local_rx_buf[0];
        // printf("RSSI: %d\n\r",rssi);
        if(len!=0)
        {
            for(i=0;i<len;i++)
                //printf("%d", local_rx_buf[i]);
                //printf("\r\n");
                //Check the type of message and decide what to do ?????
                switch(local_rx_buf[1])
                {
                case SLAVE_DISCOVERY:
                    if(rssi > RSSI_THRESHOLD)
                    {
                        addToNeighbor(local_rx_buf);
                    }
                    break;
                case NEIGHBOR_MSG:
                    //    printf("Received Neighbor Message");
                    if(rssi > RSSI_THRESHOLD)
                    {
                        processNeighborMsg(local_rx_buf, len);
                    }
                    break;
								case 3:
									if(rssi > RSSI_THRESHOLD)
									{
										if(slavesList[GATEWAY_ID]==0)
											 route_to_Gateway(local_rx_buf);
									}
									break;
                default:
                    //Do nothing as of now
                    break;
                }
        }
        bmac_rx_pkt_release ();
    }
}



void tx_task ()
{
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
			       light_Value=read_light_sensor();
			       printf("Light value %d\r\n",light_Value);
	           temp_Value=read_temp_sensor();
             tx_buf[0] = MY_ID;
             tx_buf[1] = 3;
             tx_buf[2]=GATEWAY_ID;
			       tx_buf[3]=sequenceNumber;
		         sequenceNumber++;
			       
						 memcpy(tx_buf+4,&light_Value,16);
			       memcpy(tx_buf+6,&temp_Value,16);
			       for(int k=1;k<MAX_SLAVES;k++){   // sending neighbors list 
							  if(slavesList[k]!=0)
									tx_buf[7+k]=k;
							}
             nrk_led_toggle(BLUE_LED);
             val=bmac_tx_pkt(tx_buf, RF_MAX_PAYLOAD_SIZE);
             //printf("Transmitted packet");
             nrk_wait_until_next_period ();   
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


    TX_TASK.task = tx_task;
    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
    TX_TASK.prio = 1;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = 10;
    TX_TASK.period.nano_secs = 0;
    TX_TASK.cpu_reserve.secs = 0;
    TX_TASK.cpu_reserve.nano_secs = 600 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);

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

void route_to_Gateway(uint8_t* rx_buf){
	nrk_sig_t tx_done_signal;
	tx_done_signal = bmac_get_tx_done_signal ();
	int destinationNode=rx_buf[2],val;
   nrk_signal_register (tx_done_signal);
	
	for(int l=1; l <= MAX_SLAVES;l++){
		if(routingTable[destinationNode][l]!=0)
			rx_buf[1]=l; //destination is the next hop
		break;
  }
	val=bmac_tx_pkt((char*)rx_buf, RF_MAX_PAYLOAD_SIZE);
	printf("Transmitted to the next hop\r\n");
}