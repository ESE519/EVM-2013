#include <include.h>
#include <ulib.h>
#include <stdio.h>

typedef enum states{
		IDLE_STATE,
		GOT_TASK_PARAM,
		SAVED_FUNC_NAME,
		SAVED_FUNC_CODE,
		SAVED_INITIAL_STATES,
		ACTIVATED_TASK,
		DEACTIVATED_TASK
	}States;


typedef enum signals{
	ACK_SIGNAL,
	NACK_SIGNAL,
	NO_SIGNAL
}Signals;

Signals slave_sm(uint8_t);