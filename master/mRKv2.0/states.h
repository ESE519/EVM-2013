
typedef enum TOP_LEVEL_STATE {
	START,
	ASSIGN_TASKS,
	STEADY_STATE
};


typedef enum TOP_LEVEL_SIGNAL {
	FOUND_NEIGHBORS,
	NODE_DIED,
	ASSGN_DONE,
	NO_SIGNAL_TOP
};


typedef enum MIDDLE_LEVEL_STATE {
	IDLE,
	FIND_NEXT_NODE,
	SEND_TASK_MID
};

typedef enum MIDDLE_LEVEL_SIGNAL {
	FIND_NODE_SIGNAL,
	NODE_FOUND_SIGNAL,
	SENDING_DONE,
	NO_SIGNAL_MIDDLE
};


typedef enum LOW_LEVEL_STATE {
	IDLE_LOW,
	TASK_PARAMS,
	FUNC_NAMES,
	FUNC_CODE,
	INIT_STATE,
	DEACTIVATE,
	ACTIVATE
};


typedef enum LOW_LEVEL_SIGNAL {
	SEND_TASK_LOW,
	NACK,
	ACK,
	NO_SIGNAL_LOW
};



void send_middle_level_signal (MIDDLE_LEVEL_SIGNAL sig);
void send_low_level_signal (LOW_LEVEL_SIGNAL sig);
void send_top_level_signal (TOP_LEVEL_SIGNAL sig);