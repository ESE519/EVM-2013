#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H


#include "IAP.h"
#include "mbed.h"
#include <inttypes.h>






int set_start_address(uint32_t address);
uint32_t get_start_address();

int set_end_address(uint32_t address);
uint32_t get_end_address();

char * copy_code_flash(char *array, unsigned int size, int *errno);

#endif
