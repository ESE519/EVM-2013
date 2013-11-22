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

//#include "jumptable.h"
#include "memory_manager.h"
#include "function_manager.h"
#include "messagetypes.h"


// Only require MAC address for address decode
//#define MAC_ADDR    0x0001

// Change this to your group channel
#define MY_CHANNEL 2

#define MAX_MOLES  2
#define MY_ID 2
#define TARGET_SECTOR       14
//#define THRESHOLD    3750

// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...


typedef int (*function) ();
function copied_function;

typedef void (*functionTask) ();

//Create the required tasks 
nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

//Create the dummy tasks required which will execute the tasks 

nrk_task_type DUMMY_TASK;
NRK_STK dummy_task_stack[NRK_APP_STACKSIZE];
void dummy(void);
functionTask dummyFunction;

nrk_task_type DUMMY_TASK1;
NRK_STK dummy1_task_stack[NRK_APP_STACKSIZE];
void dummy1(void);
functionTask dummy1Function;

nrk_task_type DUMMY_TASK2;
NRK_STK dummy2_task_stack[NRK_APP_STACKSIZE];
void dummy2(void);
functionTask dummy2Function;





void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];

char enableTask[3]={0};
uint32_t functionSize;
uint8_t functionRecieved;
uint8_t startAddress;
uint32_t code_aligned[1024];
void get_functionCode(char *cptr);

//Value to determine the number of dummy tasks running
uint8_t numberDummyTasks;


//struct Jump_Table_Function 		table_function  __attribute__((at(0x10005000))); 

//Dummy function which is created to execute a task
void dummy(void)
{
while(1)
{	

if(enableTask[0]==1)
{
dummyFunction();
}

nrk_wait_until_next_period();
}
}

//Dummy function which is created to execute a task
void dummy1(void)
{
while(1)
{	

	if(enableTask[1]==1)
	{
	dummy1Function();
	}

	nrk_wait_until_next_period();
	}
}

//Dummy function which is created to execute a task
void dummy2(void)
{
	while(1)
	{	
	if(enableTask[2]==1)
	{
	dummy2Function();
	}

	nrk_wait_until_next_period();
	}
}




int main(void)

{
	int r;
	uint32_t a;
		function_manager_init();
		a = *((uint32_t *)0x10006000);
	  printf("address of get handle is %X\n\r", a);
		function_register("toggle", strlen("toggle") + 1, (char *)&nrk_led_toggle);
    function_register("print", strlen("print") + 1, (char *)&printf);
		nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    
		
    r = set_start_address(0xE000);
    
    if(r) 
    	printf("start address setting failed\n\r");
    
    r = set_end_address(0xF000);
    if(r)
    	printf("end address setting failed\n\r");
    	
    nrk_start();
    return 0;

}

void rx_task ()
{ 
    uint8_t rssi,len,*local_rx_buf,mole, from, received_round;;
    int i,r; 
		char *fn_address;
    bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer (rx_buf,107);
    //cleaning up the target sector in the flash
    char *code;
	  code = (char *) code_aligned;
	
    while(!bmac_started());
    printf("Receiver node Bmac initialised\n");
  
        while(1) {
            nrk_wait_until_next_period();
            
					  if( !bmac_rx_pkt_ready())
                continue;
            //printf("received packet\n\r");
            //nrk_led_toggle(ORANGE_LED);
            local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);
            
            /*
            for(i=0;i<2;i++)
							printf("0x%X", local_rx_buf[i]);
                             
            printf("\r\n");
						for(i = 2; i < len; i++)
							printf("0x%X ", local_rx_buf[i]);
             */               
           if(local_rx_buf[0] ==1 && local_rx_buf[2]==MY_ID)
					 {						 
           switch(local_rx_buf[1])
						{
						
						case TASKTX:
						// getting function code
            for(i = 0; i < 200; i++){
							code[i] = local_rx_buf[i+6];
							printf("0x%X \t",code[i]);
 						}
            //getting function size
            functionSize = 256;
           
						fn_address = copy_code_flash(code, functionSize, &r);
           
						functionRecieved = 0;
						bmac_rx_pkt_release ();   
						
						if(fn_address) {	
						switch (numberDummyTasks)
						{
						case 0:
						dummyFunction=(functionTask) ((uint32_t)fn_address | 1);    
						printf("successfully copied\n\r");
						printf("Copied function address: %x\n\r", fn_address);
						DUMMY_TASK.period.secs = local_rx_buf[3];
						DUMMY_TASK.period.nano_secs =0;
						DUMMY_TASK.cpu_reserve.secs = 0;
						DUMMY_TASK.cpu_reserve.nano_secs = local_rx_buf[4] * NANOS_PER_MS;
						DUMMY_TASK.offset.secs = 0;
						DUMMY_TASK.offset.nano_secs = 0;
						r=nrk_activate_task (&DUMMY_TASK);
						if(r==1)
						{
						printf("successfully Initiated a task 1\n\r");
						enableTask[numberDummyTasks]=1;
						numberDummyTasks++;
						functionRecieved=1;
						}
						else
						printf("Could Not Initate the task\n\r");
						break;
						case 1:
						dummy1Function=(functionTask) ((uint32_t)fn_address | 1);    
						printf("successfully copied\n\r");
						printf("Copied function address: %x\n\r", fn_address);
						DUMMY_TASK1.period.secs = local_rx_buf[3];
						DUMMY_TASK1.period.nano_secs =0;
						DUMMY_TASK1.cpu_reserve.secs = 0;
						DUMMY_TASK1.cpu_reserve.nano_secs = local_rx_buf[4] * NANOS_PER_MS;
						DUMMY_TASK1.offset.secs = 0;
						DUMMY_TASK1.offset.nano_secs = 0;
						r=nrk_activate_task (&DUMMY_TASK1);
						if(r==1)
						{
						printf("successfully Initiated a task 2\n\r");
						enableTask[numberDummyTasks]=1;
						numberDummyTasks++;
						functionRecieved=1;
						}
						else
						printf("Could Not Initate the task\n\r");
						break;
						case 2:
						dummy2Function=(functionTask) ((uint32_t)fn_address | 1);    
						printf("successfully copied\n\r");
						printf("Copied function address: %x\n\r", fn_address);
						DUMMY_TASK2.period.secs = local_rx_buf[3];
						DUMMY_TASK2.period.nano_secs =0;
						DUMMY_TASK2.cpu_reserve.secs = 0;
						DUMMY_TASK2.cpu_reserve.nano_secs = 50 * NANOS_PER_MS;
						DUMMY_TASK2.offset.secs = 0;
						DUMMY_TASK2.offset.nano_secs = 0;
						r=nrk_activate_task (&DUMMY_TASK2);
						if(r==1)
						{
						printf("successfully Initiated a task\n\r");
						enableTask[numberDummyTasks]=1;
						numberDummyTasks++;
						functionRecieved=1;
						}
						else
						printf("Could Not Initate the task\n\r");
						break;
						default:
						tx_buf[1] = TASKNACK;	
						break;

					}

					


						
				}
					
				else {
				printf("IAP copy error: %d\n\r", r);
				
				}
				 //calling the copi8ed function
						
						
        }
			}
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
			
						if(functionRecieved) {
							tx_buf[0] = MY_ID;
							tx_buf[1] = TASKACK ;
							length = 2;
							val=bmac_tx_pkt(tx_buf, length);
							//printf("calling funtion\r\n");
							//r = copied_function();
							printf("TX",r);
							functionRecieved=0;
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
    RX_TASK.period.nano_secs = 100* NANOS_PER_MS;
    RX_TASK.cpu_reserve.secs = 0;
    RX_TASK.cpu_reserve.nano_secs = 50 * NANOS_PER_MS;
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
    TX_TASK.cpu_reserve.nano_secs = 50 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);

    DUMMY_TASK.task = dummy;
		nrk_task_set_stk( &DUMMY_TASK, dummy_task_stack, NRK_APP_STACKSIZE);
		DUMMY_TASK.prio = 1;
		DUMMY_TASK.FirstActivation = TRUE;
		DUMMY_TASK.Type = BASIC_TASK;
		DUMMY_TASK.SchType = PREEMPTIVE;
		DUMMY_TASK.period.secs = 10;
		DUMMY_TASK.period.nano_secs = 0;
		DUMMY_TASK.cpu_reserve.secs = 0;
		DUMMY_TASK.cpu_reserve.nano_secs = 0;
		DUMMY_TASK.offset.secs = 0;
		DUMMY_TASK.offset.nano_secs = 0;
		//nrk_activate_task (&DUMMY_TASK);
		
		DUMMY_TASK1.task = dummy1;
		nrk_task_set_stk( &DUMMY_TASK1, dummy1_task_stack, NRK_APP_STACKSIZE);
		DUMMY_TASK1.prio = 1;
		DUMMY_TASK1.FirstActivation = TRUE;
		DUMMY_TASK1.Type = BASIC_TASK;
		DUMMY_TASK1.SchType = PREEMPTIVE;
		DUMMY_TASK1.period.secs = 10;
		DUMMY_TASK1.period.nano_secs = 0;
		DUMMY_TASK1.cpu_reserve.secs = 0;
		DUMMY_TASK1.cpu_reserve.nano_secs = 0;
		DUMMY_TASK1.offset.secs = 0;
		DUMMY_TASK1.offset.nano_secs = 0;
		
		
		DUMMY_TASK2.task = dummy2;
		nrk_task_set_stk( &DUMMY_TASK2, dummy2_task_stack, NRK_APP_STACKSIZE);
		DUMMY_TASK2.prio = 1;
		DUMMY_TASK2.FirstActivation = TRUE;
		DUMMY_TASK2.Type = BASIC_TASK;
		DUMMY_TASK2.SchType = PREEMPTIVE;
		DUMMY_TASK2.period.secs = 10;
		DUMMY_TASK2.period.nano_secs = 0;
		DUMMY_TASK2.cpu_reserve.secs = 0;
		DUMMY_TASK2.cpu_reserve.nano_secs = 0;
		DUMMY_TASK2.offset.secs = 0;
		DUMMY_TASK2.offset.nano_secs = 0;
		

    printf ("Create done\r\n");
}
