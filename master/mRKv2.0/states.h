
typedef enum  {
	STATE_START,
	ASSIGN_TASKS,
	STEADY_STATE
}TOP_LEVEL_STATE;


typedef enum  {
	FOUND_NEIGHBORS,
	NODE_DIED,
	ASSGN_DONE,
	NO_SIGNAL_TOP
}TOP_LEVEL_SIGNAL;


typedef enum  {
	IDLE,
	FIND_NEXT_NODE,
	SEND_TASK_MID
}MIDDLE_LEVEL_STATE;

typedef enum  {
	FIND_NODE_SIGNAL,
	NODE_FOUND_SIGNAL,
	SENDING_DONE,
	NO_SIGNAL_MIDDLE
}MIDDLE_LEVEL_SIGNAL;


typedef enum  {
	IDLE_LOW,
	TASK_PARAMS,
	FUNC_NAMES,
	FUNC_CODE,
	INIT_STATE,
	DEACTIVATE,
	ACTIVATE,
	BLOCK_SEND
}LOW_LEVEL_STATE;


typedef enum  {
	SEND_TASK_LOW,
	NACK,
	ACK,
	NO_SIGNAL_LOW
}LOW_LEVEL_SIGNAL;



void send_middle_level_signal (MIDDLE_LEVEL_SIGNAL sig);
void send_low_level_signal (LOW_LEVEL_SIGNAL sig);
void send_top_level_signal (TOP_LEVEL_SIGNAL sig);

void middle_level_take_action ();
void low_level_take_action ();