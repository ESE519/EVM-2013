#include "mbed.h"

DigitalOut myled(LED1);

extern "C"
void HardFault_Handler() {
    register unsigned int _msp __asm("msp");
    printf("Hard Fault! %x (%x)\r\n", SCB->HFSR, *((unsigned int *)(_msp + 24)));
    printf("HFSR: 0x%X\n\r", SCB->HFSR);
    printf("MMFAR: 0x%X\tMMFSR: 0x%X\n\r", SCB->MMFAR, SCB->CFSR);
    printf("BFAR: 0x%X\tBFSR: 0x%X\n\r", SCB->BFAR, SCB->CFSR);
    printf(" - %x\r\n", (*(volatile uint32_t*)0xe000ed24));
//    printf("Hard Fault! %x\r\n", SCB->HFSR);
    exit(-1);
}


void foo();

int main() {
	  myled = 1;
    while(1) {
       foo();
    }
}
