
#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <map>
#include <string>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>
#include <string.h>
#include "mbed.h"
#include "basic_rf.h"
#include "bmac.h"
#include "IAP.h"
#include "jumptable.h"

// Only require MAC address for address decode
//#define MAC_ADDR    0x0001

// Change this to your group channel
#define MY_CHANNEL 2

#define MAX_MOLES  2

#define MAX_RETRANSMISSION        10
#define MAX_TTL 30;

// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...
#define MY_ID 1

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type SD_TASK;
NRK_STK sd_task_stack[NRK_APP_STACKSIZE];
void slaveDiscovery_task (void);

void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
void print_function(char *ptr, int num);


volatile uint8_t received_response = 0;

char code[1024];
IAP     iap;

/******* time definitions *******/
nrk_time_t timeend, timeout, timestart;
int simple_function();
void copy_code_ram();
uint32_t anotherScore = 0;


struct Jump_Table_Function 		table_function  __attribute__((at(0x10005000))); 


std::map<int , uint8_t> slavesMap;
void checkSlaves(char*);
void sendHelloMessage();
void ReduceTTL();

int main(void)

{   
    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    nrk_start();
    return 0;

}


void ReduceTTL(){
	for (std::map<int,uint8_t >::iterator i =
         slavesMap.begin (); i != slavesMap.end (); i++){
			i->second--;
			if(i->second==0)
				slavesMap.erase(i);
				 }
}


/** Sending Hello Messages to check slaves **/
void sendHelloMessage(){
	uint8_t val;
	tx_buf[0]=1;
	tx_buf[1]=10;
	
	val=bmac_tx_pkt(tx_buf,2);
	
}



void slaveDiscovery_task(){
  printf("Slave Discovery task scheduled");
	sendHelloMessage();	
}

void rx_task ()
{
    printf("rx task scheduled\r\n");
	  uint8_t *local_rx_buf;
    uint8_t len,rssi;
    int i,fromMole;
    bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer(rx_buf,RF_MAX_PAYLOAD_SIZE);
    while(1){
        nrk_wait_until_next_period();
        if(!bmac_rx_pkt_ready())
            continue;
				
				if(len == 0)
					continue;
        received_response=1; // an ack recieved
        local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);
			  // a hello response recieved
				if(local_rx_buf[1] == 101) {
						fromMole=local_rx_buf[0];
						int TTL=30;
						std::map<int, uint8_t>::iterator k =slavesMap.find(fromMole);
						
						if(k!=slavesMap.end()){ 
							 k->second=MAX_TTL;
							}
			      else // a new neighbor recieved
								slavesMap.insert(std::make_pair(fromMole,TTL));
					}   
						if(len != 0) {
            //printing out the contents recieved from the sender node
            for(i = 0; i < len; i++)
                printf("%d ", local_rx_buf[i]);
            printf("\r\n");
         }     
    }
		bmac_rx_pkt_release ();
		
}


void tx_task ()
{   printf("Sender task scheduled");
	  ReduceTTL(); // reduce TTL for enteries in the neighbor table
    printf("tx task scheduled\r\n");
    nrk_sig_t tx_done_signal;
    uint8_t val, sequenceNumber;
    nrk_time_t r_period;   
	   	
    while (!bmac_started ())
        nrk_wait_until_next_period ();
         printf("Sender node bmac initialised\r\n");
        
        tx_done_signal = bmac_get_tx_done_signal ();
        nrk_signal_register (tx_done_signal);
        sendHelloMessage(); 
		 while(1) {
        if(!received_response){
            tx_buf[0] = MY_ID; 
            tx_buf[1] = 256; //function Size
		 
            					
            nrk_led_toggle(ORANGE_LED);
            val=bmac_tx_pkt(tx_buf, 102);
            //printf("Transmitted packet");   
            nrk_wait_until_next_period ();
        }
			}
       
    }       
   
   
 



void nrk_create_taskset ()
{
		SD_TASK.task = slaveDiscovery_task;
    nrk_task_set_stk( &SD_TASK, sd_task_stack, NRK_APP_STACKSIZE);
    SD_TASK.prio = 3;
    SD_TASK.FirstActivation = TRUE;
    SD_TASK.Type = BASIC_TASK;
    SD_TASK.SchType = PREEMPTIVE;
    SD_TASK.period.secs = 2;
    SD_TASK.period.nano_secs = 0;
    SD_TASK.cpu_reserve.secs = 0;
    SD_TASK.cpu_reserve.nano_secs = 300 * NANOS_PER_MS;
    SD_TASK.offset.secs = 0;
    SD_TASK.offset.nano_secs = 0;
    nrk_activate_task (&SD_TASK);

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
    TX_TASK.period.secs = 3;
    TX_TASK.period.nano_secs = 0;
    TX_TASK.cpu_reserve.secs = 0;
    TX_TASK.cpu_reserve.nano_secs = 600 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);
		
    printf ("Create done\r\n");
}