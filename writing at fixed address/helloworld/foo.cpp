#include "mbed.h"

DigitalOut led(LED2);

void foo() {
	printf("not in foo\n\r");
	led = 1;
	wait(0.2);
	led = 0;
	wait(0.2);
}