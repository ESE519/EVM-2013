#include "mbed.h"
#include "IAP.h"

#define TARGET_SECTOR       14

DigitalOut myled(LED1);
char code[1024];
IAP     iap;
char val = 0;


typedef int (*function) ();
function blinkfunction;

MPU_Type mpu;
int foo();


extern "C"
void HardFault_Handler() {
    register unsigned int _msp __asm("msp");
    printf("Hard Fault! %x (%x)\r\n", SCB->HFSR, *((unsigned int *)(_msp + 24)));
    printf("HFSR: 0x%X\n\r", SCB->HFSR);
    printf("MMFAR: 0x%X\tMMFSR: 0x%X\n\r", SCB->MMFAR, SCB->CFSR);
    printf("BFAR: 0x%X\tBFSR: 0x%X\n\r", SCB->BFAR, SCB->CFSR);
    printf(" - %x\r\n", (*(volatile uint32_t*)0xe000ed24));
//    printf("Hard Fault! %x\r\n", SCB->HFSR);

		printf("*********** MPU Settings *************\n\r");
		printf("TYPE: 0x%X\n\r", mpu.TYPE);
		printf("CTRL: 0x%X\n\r", mpu.CTRL);
    exit(-1);
}




int blink() {
    int a = 1, b = 1;
		return a + b;
}



void copy_code_ram() {
    
    char *charptr;
    
    charptr = (char *)(0x36E);
    int i;
    for(i = 0; i <200 ; i++) {
        code[i] = *charptr;
        charptr++;
    }
}    

void print_function(char *ptr, int num) {
    for(; num > 0; num--) {
        printf("0x%X  ", *ptr);
        ptr++;
    }
}
        

int main() {
    int r;
    
		foo();

    printf("blink code:\n");
    print_function((char *)&blink, 100);
    
    printf("blink address is 0x%X\n\r", &blink);

    printf("\n\n\n\n\n");
    copy_code_ram();
		
		
    print_function(sector_start_adress[TARGET_SECTOR], 100);
    printf("\n\n\n\n\n");
		
		iap.prepare( TARGET_SECTOR, TARGET_SECTOR);
		iap.erase (TARGET_SECTOR, TARGET_SECTOR);
    iap.prepare( TARGET_SECTOR, TARGET_SECTOR);
    r   = iap.write( code, sector_start_adress[TARGET_SECTOR], 256);
    printf("\n\n\n\n\n");
    printf("%d",r);
    printf("\n\n\n\n\n");
    
    printf( "copied: SRAM(0x%08X)->Flash(0x%08X) for %d bytes. (result=0x%08X)\r\n", code, sector_start_adress[ TARGET_SECTOR ], 1024, r );
    printf("\n\n\n\n\n");
    
    blinkfunction = (function) (0xE000 | 1);

    printf("\n\n\n\n\n");
    print_function((char *)&foo , 100);
    
    printf("copied function \n");
		
		r = 0;
		r = blink();
		printf("The return value from blink is %d\n\r", r);
		
		printf("The address of foo is 0x%X\n\r", &foo);
    r = blinkfunction();
		printf("The return value from blinkfunction is %d\n\r", r);
    while(1) {
       

    }
}



