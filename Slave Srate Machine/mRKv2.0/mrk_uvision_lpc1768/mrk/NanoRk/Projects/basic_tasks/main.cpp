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
#include "messageTypes.h"

Serial pc(USBTX, USBRX);




NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void Task1(void);

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;
void Task2 (void);

NRK_STK Stack3[NRK_APP_STACKSIZE];
nrk_task_type SR;
void serialReceive(void);

volatile int task1val=0,task2val=0;


void nrk_create_taskset();

// You do not need to modify this function
struct __FILE { int handle; };

int main(void)

  {
			int i;
    	nrk_setup_ports();
			nrk_init();
			nrk_create_taskset();
			nrk_start();
			return 0;

	}


	void Task1()

	{
		
		while(1)
		{
			static int i = 1;
			nrk_led_toggle(ORANGE_LED);
			
			printf("Task 1 count = %d\n\r",packetHeaderVal++);
			
			nrk_wait_until_next_period();
		}
		
	}

	
	
	void Task2()
	{
		
		while(1)
		{
			static int i = 1;
			nrk_led_toggle(BLUE_LED);
			
			printf("Task 2 count = %d\n\r",task2val);
			
			nrk_wait_until_next_period();
		}
		
	}
void serialReceive(void)
{
	char c=0;
	while(1)
	{
				
          
        c= pc.getc(); 
					if(c)
					{
						nrk_led_set(RED_LED);
					printf("HAHAHA\n");
					}
				nrk_wait_until_next_period();
	}
	
	
}
void nrk_create_taskset()

{
	
    nrk_task_set_entry_function( &TaskOne, Task1);
    nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
    TaskOne.prio = 1;
    TaskOne.FirstActivation = TRUE;
    TaskOne.Type = BASIC_TASK;
    TaskOne.SchType = PREEMPTIVE;
    TaskOne.period.secs = 1;
    TaskOne.period.nano_secs = 0;
    TaskOne.cpu_reserve.secs = 0;
    TaskOne.cpu_reserve.nano_secs = 200*NANOS_PER_MS;
    TaskOne.offset.secs = 0;
    TaskOne.offset.nano_secs= 0;
    nrk_activate_task (&TaskOne);

		nrk_task_set_entry_function( &TaskTwo, Task2);
		nrk_task_set_stk( &TaskTwo, Stack2, NRK_APP_STACKSIZE);
		TaskTwo.prio = 2;
		TaskTwo.FirstActivation = TRUE;
		TaskTwo.Type = BASIC_TASK;
		TaskTwo.SchType = PREEMPTIVE;
		TaskTwo.period.secs = 3;
		TaskTwo.period.nano_secs = 0;
		TaskTwo.cpu_reserve.secs = 0;
		TaskTwo.cpu_reserve.nano_secs = 200*NANOS_PER_MS;
		TaskTwo.offset.secs = 0;
		TaskTwo.offset.nano_secs= 0;
		nrk_activate_task (&TaskTwo);
		

		nrk_task_set_entry_function( &SR, serialReceive);
		nrk_task_set_stk( &SR, Stack3, NRK_APP_STACKSIZE);
		SR.prio = 10;
		SR.FirstActivation = TRUE;
		SR.Type = BASIC_TASK;
		SR.SchType = NONPREEMPTIVE;
		SR.period.secs = 10;
		SR.period.nano_secs = 0;
		SR.cpu_reserve.secs = 0;
		SR.cpu_reserve.nano_secs = 2*NANOS_PER_MS;
		SR.offset.secs = 0;
		SR.offset.nano_secs= 0;
		nrk_activate_task (&SR);
	
	
	
	
}
