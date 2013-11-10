
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
//#include "IAP.h"
//#include "jumptable.h"

#include "function_manager.h"

// Only require MAC address for address decode
//#define MAC_ADDR    0x0001

// Change this to your group channel
#define MY_CHANNEL 2

#define MAX_MOLES  2

#define MAX_RETRANSMISSION        10


// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...
#define MY_ID 1

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

void nrk_create_taskset ();

char tx_buf[102];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
void print_function(char *ptr, int num);


volatile uint8_t received_response = 0;


char code[1024];


/******* time definitions *******/
nrk_time_t timeend, timeout, timestart;
int simple_function();
void copy_code_ram();
uint32_t anotherScore = 0;


int main(void)

{
		
    printf("simple_function address is 0x%X\n\r", &simple_function);
    copy_code_ram();
    print_function((char *)&simple_function, 100);
    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    nrk_start();
    return 0;

}

int simple_function(){
	int a = 1, b = 2;
	uint32_t addrh, addrl;
	
	void *(*get_function_handle)(const char *, int);
	int8_t (*led_toggle)(int);
		
	addrh = GET_HANDLE_ADDRESS_H;
	addrl = GET_HANDLE_ADDRESS_L;
	
	get_function_handle = (void *(*)(const char *, int)) ((addrh << 16) | addrl);
	led_toggle = (int8_t (*)(int)) get_function_handle("toggle", 7);
	
	led_toggle(ORANGE_LED);
	
	return (a + b);
	
}

/** Copying code to ram from flash**/

void copy_code_ram() {   
    char *charptr;   
	  uint32_t address = (uint32_t) &simple_function;
    charptr = (char *)(address & ~(1));
    int i;
    for(i = 0; i <200 ; i++) {
        code[i] = *charptr;
        charptr++;
    }
}

/**printing out function code **/
void print_function(char *ptr, int num) {
    for(; num > 0; num--) {
        printf("0x%X  ", *ptr);
        ptr++;
    }
}

void rx_task ()
{
    printf("rx task scheduled\n");
    uint8_t *local_rx_buf;
    uint8_t len,rssi;
    int i;
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
        if(len != 0) {
            //printing out the contents recieved from the sender node
            for(i = 0; i < len; i++)
                printf("%d ", local_rx_buf[i]);
            printf("\n\r");
         }     
    }
		bmac_rx_pkt_release ();
		
}


void tx_task ()
{  
    printf("tx task scheduled\n");
    nrk_sig_t tx_done_signal;
    uint8_t val, sequenceNumber;
    nrk_time_t r_period;   
		
    while (!bmac_started ())
        nrk_wait_until_next_period ();
         printf("Sender node bmac initialised\n");
       
        tx_done_signal = bmac_get_tx_done_signal ();
         nrk_signal_register (tx_done_signal);
      while(1) {
        if(!received_response){
            tx_buf[0] = MY_ID; 
            tx_buf[1] = 256; //function Size
						//tx_buf[2]= 100;  
            					
            for (int k=0; k<100;k++){
							tx_buf[k+2]=code[k];
            }
            /*
            for (int k=0; k<2;k++){
                    printf("%d\r\n",tx_buf[k]);
            }
           */
            nrk_led_toggle(ORANGE_LED);
            val=bmac_tx_pkt(tx_buf, 102);
            //printf("Transmitted packet");   
            nrk_wait_until_next_period ();
        }
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
    TX_TASK.cpu_reserve.nano_secs = 900 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);

    printf ("Create done\r\n");
}