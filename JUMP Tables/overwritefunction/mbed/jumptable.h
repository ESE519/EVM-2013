#include "mbed.h"

struct Jump_Table_Function {
	void (*wait)(float);
	int (*printf)(const char *format, ...);
	void (*blinky) ();
};



struct Jump_Table_Data {
	DigitalOut *led;
};
