#ifndef FUNCTION_MANAGER_H
#define	FUNCTION_MANAGER_H

#include "mbed.h"
#include <inttypes.h>
#include "state_manager.h"

#define GET_HANDLE_ADDRESS				0x10006000
#define GET_HANDLE_ADDRESS_H			0x1000
#define GET_HANDLE_ADDRESS_L			0x6000
#define MAX_NAME_LENGTH								20

#define MAX_NUM_TASKS_IN_NETWORK			10
#define	MAX_REF_PER_TASK							10

struct function_info {
	char name[MAX_NAME_LENGTH];            
	void *fn_ptr;
};


struct task_function_info {
	char name[MAX_NAME_LENGTH];
	uint8_t assigned_node_id;
	char fun_references[10][MAX_NAME_LENGTH];
	int num_references;
	uint32_t states[MAX_NUM_STATES];
	uint16_t periods, periodms;
	uint16_t wcets, wcetms;
};



void function_manager_init();

int function_register(const char *name, int namelen, char *address);

void * get_function_handle(const char *name, int namelen);

void print_all_function_names();



int task_function_register(	const char *name, 
														int namelen, 
														char *address);

int register_reference(const char *task_name, const char *fun_name, int fn_namelen);

int set_scheduling_parameters(const char *task_name, uint16_t ps, uint16_t pms, uint16_t es, uint16_t ems);

int find_unassigned_tasks();
int find_first_unassigned_task();

#endif