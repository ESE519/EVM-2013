#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "mbed.h"
#include <inttypes.h>

#define MAX_NUM_STATES			10

int get_state(int task_num, uint8_t pos, uint32_t *ptr);

int checkpoint_state(int task_num, uint8_t pos, uint32_t val);

int store_states(int task_num, uint32_t *data);
#endif