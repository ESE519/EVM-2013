#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <string.h>

enum Message{
 TASKTX=1,
 TASKACK=2,
 TASKNACK=3	
};

	MessageTypes
	
}
enum MessageTypes{
	PING=1,
	TASK_PARAMS=2,
	FUNC_NAMES=3,
	FUNC_CODE=4,
	INIT_STATE=5,
	ACTIVATE=6,
	CHECKPOINT=7,
	ACK=8,
	NACK=9,
}mtype;

struct messages{
	
	
};
struct Pingmessage{
	
	
};

struct Pingresponse{
	
};

struct TaskParams{
	uint16_t period_sec;
	uint16_t period_ms;
	uint16_t wcet_sec;
	uint16_t wcet_ms;
};

struct FuncCode{
	char name;
	uint16_t size;
	char code[1024];
};

struct Init_States{
	uint8_t numofStates;
	uint32_t States[4];
};

struct Activate{
	
};

struct CheckPoint{
	uint8_t numofStates;
	uint32_t States[4];
};

struct Ack{
};

struct Nack{
};

char* Process_PingMessage(char *,char *);
void set_TaskParams(uint16_t ,uint16_t ,uint16_t ,uint16_t );
void set_FuncNames();
void set_FuncCode();
void set_PingMessage();
void checkforResponses(char*);
void sendPing();