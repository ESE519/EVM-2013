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

#include "basic_rf.h"
#include "bmac.h"
#include <ff_basic_sensor.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include "IAP.h"
#include "jumptable.h"


// Only require MAC address for address decode
//#define MAC_ADDR    0x0001

// Change this to your group channel
#define MY_CHANNEL 2

#define MAX_MOLES  2
#define MY_ID 2
#define TARGET_SECTOR       14
//#define THRESHOLD    3750

// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...
#define MY_ID 2


nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];

IAP iap;
uint8_t functionSize;
uint8_t functionRecieved;
uint8_t startAddress;
char code[4096];
void get_functionCode(char *cptr);
typedef int (*function) ();
function copied_function;
int fromNode;
void sendHelloRsp();
struct Jump_Table_Function 		table_function  __attribute__((at(0x10005000))); 

int main(void)

{
		table_function.nrk_led_toggle = &nrk_led_toggle;
    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    nrk_start();
    return 0;

}

void sendHelloRsp(){
	uint8_t val;
	tx_buf[0] = MY_ID;
	tx_buf[1] = 101;
  val=bmac_tx_pkt(tx_buf, 2);
	printf("Hello Response sent");	
}
void rx_task ()
{ 
    uint8_t rssi,len,*local_rx_buf,mole, from, received_round;;
    int i,r,fromMole; 
	  bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer (rx_buf,RF_MAX_PAYLOAD_SIZE);
    
    while(!bmac_started());
    printf("Receiver node Bmac initialised\n");
  
        while(1) {
            nrk_wait_until_next_period();
            
					  if( !bmac_rx_pkt_ready())
                continue;
            printf("received packet\n\r");
            nrk_led_toggle(ORANGE_LED);
            local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);
            fromNode=local_rx_buf[0];
            
            for(i=0;i<len;i++)
							printf("%d", local_rx_buf[i]);
            printf("\r\n"); 
						
						if(fromNode==1){
							printf("Hello Response Received");
							sendHelloRsp();
						 }							 
						
					 bmac_rx_pkt_release ();   
					 functionRecieved=1;
          
        } 
				 // pointing the function pointer to the copied code in the flash
         
}



void tx_task ()
{
		int r;
    uint8_t length, val;
  
    nrk_sig_t tx_done_signal;
    nrk_sig_mask_t ret;
    nrk_time_t r_period;
    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);     
    while(1) {
						nrk_wait_until_next_period();
			
							 
							tx_buf[0] = MY_ID;
							tx_buf[1] = 100;
							length = 2;
							val=bmac_tx_pkt(tx_buf, length);
							
            }

 }
  



void nrk_create_taskset ()
{


    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 2;
    RX_TASK.FirstActivation = TRUE;
    RX_TASK.Type = BASIC_TASK;
    RX_TASK.SchType = PREEMPTIVE;
    RX_TASK.period.secs = 1;
    RX_TASK.period.nano_secs = 0;
    RX_TASK.cpu_reserve.secs = 0;
    RX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
    RX_TASK.offset.secs = 0;
    RX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&RX_TASK);

    TX_TASK.task = tx_task;
    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
    TX_TASK.prio = 1;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = 1;
    TX_TASK.period.nano_secs = 0;
    TX_TASK.cpu_reserve.secs = 0;
    TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);

    printf ("Create done\r\n");
}