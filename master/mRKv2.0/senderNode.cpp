
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

// Change this to your group channel
#define MY_CHANNEL 2


#define MAX_RETRANSMISSION        10


// Gateway has ID = 1, the slaves/moles have ID 2,3 onwards...
#define MY_ID 1

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type SR_TASK;
NRK_STK sr_task_stack[NRK_APP_STACKSIZE];
void serial_task (void);


nrk_task_type TOP_LEVEL_STATE_MACHINE_TASK;
NRK_STK top_level_sm_task_stack[NRK_APP_STACKSIZE];
void top_level_sm_task (void);


void nrk_create_taskset ();

char tx_buf[107];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
void print_function(char *ptr, int num);


volatile uint8_t received_response = 0;


char code[1024];



int simple_function(int task_num);
void copy_code_ram();


/*****************************************/
uint8_t taskTxNode=2,taskPeriod=10,taskExecution=100,ledNumber=0; //Reading these values from the user 
char enableFnTx=1;


/************* SIGNALS ******************/
TOP_LEVEL_SIGNAL my_signal = NO_SIGNAL_TOP;



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


int simple_function_setup() {
	int ret_val;
	ret_val = task_function_register("simple_function", strlen("simple_function") + 1, (char *)&simple_function);
	if(!ret_val)
		return ret_val;
	
	ret_val = register_reference("simple_function", "print", strlen("print") + 1);
	if(!ret_val)
		return ret_val;
	
	ret_val = register_reference("simple_function", "toggle", strlen("toggle") + 1);
	if(!ret_val)
		return ret_val;
	
	ret_val = set_scheduling_parameters("simple_function", 1, 0, 0, 100);
	if(!ret_val)
		return ret_val;
	
	return 0;
}



int main(void)

{
		uint32_t a;
		evm_init();
	
	
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
void serial_task()
{
	
	while(1)
	{
		
		/*
		printf("Enter the node to which the task has to be transmitted\r\n");
		scanf("%d \n",&taskTxNode);
		printf("Enter the period for the task");
		scanf("%d \n",&taskPeriod);
		printf("Enter the execution time");
		scanf("%d \n",&taskExecution);
		printf("Enter led number(0-3)");
		scanf("%d \n",&ledNumber);
		//Transmit 
		if(taskTxNode != MY_ID && taskTxNode>1 && taskTxNode<=3)
			enableFnTx=1;
		*/
		
		nrk_wait_until_next_period ();
	}

	
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
	int (*get_state2)(const char *fun_name, uint8_t pos, uint32_t *ptr);
	int (*checkpoint_state2)(const char *fun_name, uint8_t pos, uint32_t val);
	
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
	
	get_state2 = (int (*)(const char *, uint8_t, uint32_t *)) get_function_handle("get_state", 12);
	if(get_state == NULL)
		return 2;
	
	checkpoint_state2 = (int (*)(const char *, uint8_t, uint32_t)) get_function_handle("checkpoint_state", 18);
	if(checkpoint_state == NULL)
		return 3;
	/*************************************************************************************/
	
	
	//get the current state
	ret_val = get_state2("simple_function", 0, &task_state);
	if(ret_val != 0)
		return ret_val;
	
	
		
	led_toggle(RED_LED);
	print("current state: %d\n\r", task_state);
	
	task_state++;
	//checkpoint the state
	ret_val = checkpoint_state2("simple_function", 0, task_state);
	if(ret_val != 0)
		return ret_val;
	
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
        local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);
        if(len != 0) {
            if(local_rx_buf[0]!=MY_ID)
						{
						switch(local_rx_buf[1])
						{
						case TASKACK: 
							received_response=1;
							printf("Received ACK \n");
							break;
						case TASKNACK:
							received_response=1;
							printf("Received NACK \n");
							break;
						default:
							break;
						}
					}
					
					
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
          if(enableFnTx)
					{
            tx_buf[0] = MY_ID; 
            tx_buf[1] = TASKTX;
						tx_buf[2] = taskTxNode;
						tx_buf[3] = taskPeriod;
						tx_buf[4] = taskExecution;
						tx_buf[5] = 256; //function Size
											
            for (int k=0; k<100;k++){
							tx_buf[k+6]=code[k];
            }
            nrk_led_toggle(ORANGE_LED);
            val=bmac_tx_pkt(tx_buf, 106);
            printf("Transmitted packet\n\r");   
						enableFnTx=0;
					}
					else if (!received_response)
					{
						val=bmac_tx_pkt(tx_buf, 106);
            printf("Transmitted packet\n\r");   
					}
						
					nrk_wait_until_next_period ();
        }
}
       
       
   
   
void top_level_sm_task () {
	TOP_LEVEL_STATE my_state = START;
	
	
	while(1) {
		nrk_wait_until_next_period();
		
		switch(my_state) {
			case START:
			switch(my_signal) {
				case FOUND_NEIGHBORS:
					send_middle_level_signal(FIND_NODE_SIGNAL);
					my_state = ASSIGN_TASKS;
					my_signal = NO_SIGNAL_TOP;
					break;
				
				default:
					my_state = START;
					my_signal = NO_SIGNAL_TOP;
			}
			break;
			
			case ASSIGN_TASKS:
				switch(my_signal) {
					case ASSGN_DONE:
						my_state = STEADY_STATE;
						my_signal = NO_SIGNAL_TOP;
						break;
						
				}
			break;
				
			case STEADY_STATE:
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
				
						
   
 



void nrk_create_taskset ()
{
    /*
		SR_TASK.task = serial_task;
    nrk_task_set_stk( &SR_TASK, sr_task_stack, NRK_APP_STACKSIZE);
    SR_TASK.prio = 3;
    SR_TASK.FirstActivation = TRUE;
    SR_TASK.Type = BASIC_TASK;
    SR_TASK.SchType = NONPREEMPTIVE;
    SR_TASK.period.secs = 100;
    SR_TASK.period.nano_secs = 100* NANOS_PER_MS;
    SR_TASK.cpu_reserve.secs = 10;
    SR_TASK.cpu_reserve.nano_secs = 10 * NANOS_PER_MS;
    SR_TASK.offset.secs = 0;
    SR_TASK.offset.nano_secs = 0;
    nrk_activate_task (&SR_TASK);
		*/
	
    RX_TASK.task = rx_task;
    nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
    RX_TASK.prio = 1;
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
    TX_TASK.prio = 2;
    TX_TASK.FirstActivation = TRUE;
    TX_TASK.Type = BASIC_TASK;
    TX_TASK.SchType = PREEMPTIVE;
    TX_TASK.period.secs = 5;
    TX_TASK.period.nano_secs = 0;
    TX_TASK.cpu_reserve.secs = 0;
    TX_TASK.cpu_reserve.nano_secs = 900 * NANOS_PER_MS;
    TX_TASK.offset.secs = 0;
    TX_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TX_TASK);
		
	TOP_LEVEL_STATE_MACHINE_TASK.task = top_level_sm_task;
    nrk_task_set_stk( &TOP_LEVEL_STATE_MACHINE_TASK, top_level_sm_task_stack, NRK_APP_STACKSIZE);
    TOP_LEVEL_STATE_MACHINE_TASK.prio = 2;
    TOP_LEVEL_STATE_MACHINE_TASK.FirstActivation = TRUE;
    TOP_LEVEL_STATE_MACHINE_TASK.Type = BASIC_TASK;
    TOP_LEVEL_STATE_MACHINE_TASK.SchType = PREEMPTIVE;
    TOP_LEVEL_STATE_MACHINE_TASK.period.secs = 5;
    TOP_LEVEL_STATE_MACHINE_TASK.period.nano_secs = 0;
    TOP_LEVEL_STATE_MACHINE_TASK.cpu_reserve.secs = 0;
    TOP_LEVEL_STATE_MACHINE_TASK.cpu_reserve.nano_secs = 900 * NANOS_PER_MS;
    TOP_LEVEL_STATE_MACHINE_TASK.offset.secs = 0;
    TOP_LEVEL_STATE_MACHINE_TASK.offset.nano_secs = 0;
    nrk_activate_task (&TOP_LEVEL_STATE_MACHINE_TASK);
		
		


    printf ("Create done\r\n");
}
