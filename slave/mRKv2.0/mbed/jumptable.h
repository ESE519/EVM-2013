#include "mbed.h"

struct Jump_Table_Function {
	int8_t (*nrk_led_toggle) (int);
};



struct Jump_Table_Data {
	DigitalOut *led;
};
