#ifndef FUNCTION_MANAGER_H
#define	FUNCTION_MANAGER_H

#include "mbed.h"
#include <inttypes.h>

#define GET_HANDLE_ADDRESS				0x10006000
#define GET_HANDLE_ADDRESS_H			0x1000
#define GET_HANDLE_ADDRESS_L			0x6000
#define MAX_NAME_LENGTH								20

struct function_info {
	char name[MAX_NAME_LENGTH];            
	void *fn_ptr;
};



void function_manager_init();

int function_register(const char *name, int namelen, char *address);

void * get_function_handle(const char *name, int namelen);

void print_all_function_names();

#endif