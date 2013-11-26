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

#include "memory_manager.h"
#include "function_manager.h"
#include "messagetypes.h"
#include "state_manager.h"
#include "fn_task_map.h"


#define MY_CHANNEL 						2
#define MY_ID 								2
#define NUM_VIRTUAL_TASKS			3

#define RX_TASK_PERIOD_S			0
#define RX_TASK_PERIOD_MS			100
#define TX_TASK_PERIOD_S			1
#define	TX_TASK_PERIOD_MS			0

#define RX_TASK_RES_S					0
#define RX_TASK_RES_MS				50
#define TX_TASK_RES_S					0
#define TX_TASK_RES_MS				50


typedef int (*function) ();
function copied_function;

/********* TASK DECLARATIONS ****************************************/
nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type VIRTUAL_TASKS[NUM_VIRTUAL_TASKS];
NRK_STK virtual_tasks_stack[NUM_VIRTUAL_TASKS][NRK_APP_STACKSIZE];
void virtual_task_0();
void virtual_task_1();
void virtual_task_2();

/*******************************************************************/




/********** Function Declarations ******************************/
void evm_init();
void nrk_create_taskset ();
int is_schedulable(uint16_t period, uint16_t wcet);
uint32_t code_aligned[512];
/**************************************************************/




/****************  Global variable declarations *******************/
char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t enable_virtual_tasks[NUM_VIRTUAL_TASKS];
																				
											
/******************************************************************/






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
	const float communication_utilization = ((float)RX_TASK_RES_MS + RX_TASK_RES_S*1000)/((float)RX_TASK_PERIOD_MS + RX_TASK_PERIOD_S*1000)      + 
																				((float)TX_TASK_RES_MS + TX_TASK_RES_S*1000)/((float)TX_TASK_PERIOD_MS + TX_TASK_PERIOD_S*1000); 

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




int main(void)

{
	int r, i;
	uint32_t a;
	void *p;
	
	evm_init();
	r = function_register("toggle", strlen("toggle") + 1, (char *)&nrk_led_toggle);
	if(r!=0)
			printf("Error: cant register toggle\n\r");
	
	r = function_register("print", strlen("print") + 1, (char *)&printf);
	if(r!=0)
		printf("Error: cant register print\n\r");
	
	nrk_setup_ports();
	nrk_init();
	bmac_task_config();
	nrk_create_taskset();
	bmac_init (MY_CHANNEL);
	bmac_auto_ack_disable();
	
  printf("period: %d, wcet: %d, schedulable: %d\n\r", 10, 1, is_schedulable(10, 1));
  printf("period: %d, wcet: %d, schedulable: %d\n\r", 100, 14, is_schedulable(100, 14));
	printf("period: %d, wcet: %d, schedulable: %d\n\r", 10, 5, is_schedulable(10, 5));
	printf("period: %d, wcet: %d, schedulable: %d\n\r", 100, 1, is_schedulable(100, 1));
	
	nrk_start();
	return 0;

}

void rx_task ()
{ 
    /*uint8_t rssi,len,*local_rx_buf,mole, from, received_round;;
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
   
            local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);
            
					 
           if(local_rx_buf[0] ==1 && local_rx_buf[2]==MY_ID)
					 {						 
           switch(local_rx_buf[1])
						{
						
						case TASKTX:
						// getting function code
						for(i = 0; i < 200; i++){
							code[i] = local_rx_buf[i+6];
							//printf("0x%X \t",code[i]);
 						}
						//getting function size
						functionSize = 256;
           
						fn_address = copy_code_flash(code, functionSize, &r);
						
            map_function_task("simple_function");
						
						
						functionRecieved = 0;
						bmac_rx_pkt_release ();   
						
						if(fn_address) {	
						switch (numberDummyTasks)
						{
						case 0:
						dummyFunction=(functionTask) ((uint32_t)fn_address | 1);    
						printf("successfully copied\n\r");
						printf("Copied function address: %x\n\r", fn_address);
						r = dummyFunction();
						printf("returned %d\n\r", r);
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
		}*/
				 // pointing the function pointer to the copied code in the flash
         return;
}



void tx_task ()
{
		/*int r;
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

        }*/
		return;		
  
}


void nrk_create_taskset ()
{


    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 2;
    RX_TASK.FirstActivation = TRUE;
    RX_TASK.Type = BASIC_TASK;
    RX_TASK.SchType = PREEMPTIVE;
    RX_TASK.period.secs = RX_TASK_PERIOD_S;
    RX_TASK.period.nano_secs = RX_TASK_PERIOD_MS * NANOS_PER_MS;
    RX_TASK.cpu_reserve.secs = RX_TASK_RES_S;
    RX_TASK.cpu_reserve.nano_secs = RX_TASK_RES_MS * NANOS_PER_MS;
    RX_TASK.offset.secs = 0;
    RX_TASK.offset.nano_secs = 0;
    //nrk_activate_task (&RX_TASK);

    TX_TASK.task = tx_task;
    nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
    TX_TASK.prio = 1;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = TX_TASK_PERIOD_S;
    TX_TASK.period.nano_secs = TX_TASK_PERIOD_MS;
    TX_TASK.cpu_reserve.secs = TX_TASK_RES_S;
    TX_TASK.cpu_reserve.nano_secs = TX_TASK_RES_MS * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    //nrk_activate_task (&TX_TASK);
		
		
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
