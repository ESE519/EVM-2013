#include "mbed.h"
#include "IAP.h"
#include "jumptable.h"
#define TARGET_SECTOR       14

DigitalOut myled(LED1);
char code[1024];
IAP     iap;
char val = 0;

unsigned int blink();

typedef int (*function) ();
function blinkfunction;

MPU_Type mpu;
int foo();


struct Jump_Table_Function 		table_function  __attribute__((at(0x10005000))); 
struct Jump_Table_Data				table_data			__attribute__((at(0x10006000))); 
//table T __attribute__((at(0x10005000))); 

extern "C"
void HardFault_Handler() {
    register unsigned int _msp __asm("msp");
    printf("Hard Fault! %x (%x)\r\n", SCB->HFSR, *((unsigned int *)(_msp + 24)));
    printf("HFSR: 0x%X\n\r", SCB->HFSR);
    printf("MMFAR: 0x%X\tMMFSR: 0x%X\n\r", SCB->MMFAR, SCB->CFSR);
    printf("BFAR: 0x%X\tBFSR: 0x%X\n\r", SCB->BFAR, SCB->CFSR);
    printf(" - %x\r\n", (*(volatile uint32_t*)0xe000ed24));
    printf("Hard Fault! %x\r\n", SCB->HFSR);

		//printf("*********** MPU Settings *************\n\r");
		//printf("TYPE: 0x%X\n\r", mpu.TYPE);
		//printf("CTRL: 0x%X\n\r", mpu.CTRL);
    exit(-1);
}







void blink10times() {
	int i;
	for(i = 0; i < 10; i++) {
		myled = 1;
		wait(0.2);
		myled = 0;
		wait(0.2);
	}
	return;
}

void copy_code_ram() {
    
    char *charptr;
    
    charptr = (char *)(0x9000);
    int i;
    for(i = 0; i <1024 ; i++) {
        code[i] = *charptr;
        charptr++;
    }
}    

void print_function(char *ptr, int num) {
    for(; num > 0; num--) {
        //printf("0x%X  ", *ptr);
        ptr++;
    }
}


void prepare_jump_tables() {
	table_function.wait = &wait;
	table_function.printf = &printf;
	table_function.blinky = &blink10times;
	
	table_data.led = &myled;
	
	//printf("The address of table_functions is 0x%X\n\r", &T);
	//printf("wait: 0x%X\n\r", T.wait);
	//printf("//printf: 0x%X\n\r", T.//printf);
	//printf("blinky: 0x%X\n\r", T.blinky);
	
}
   

		

int main() {
    unsigned int r;
	
		prepare_jump_tables();
    
		
		//foo();

    ////printf("blink code:\n");
    //print_function((char *)&blink, 100);
    
    printf("\n\n\r\rblink address is 0x%X\n\r", &blink);

    //printf("\n\n\n\n\n");
    copy_code_ram();
		
		
   // print_function(sector_start_adress[TARGET_SECTOR], 100);
    ////printf("\n\n\n\n\n");
		
		iap.prepare( TARGET_SECTOR, TARGET_SECTOR);
		iap.erase (TARGET_SECTOR, TARGET_SECTOR);
    iap.prepare( TARGET_SECTOR, TARGET_SECTOR);
    r   = iap.write( code, sector_start_adress[TARGET_SECTOR], 1024);
    ////printf("\n\n\n\n\n");
    ////printf("%d",r);
    ////printf("\n\n\n\n\n");
    
    printf( "copied: SRAM(0x%08X)->Flash(0x%08X) for %d bytes. (result=0x%08X)\r\n", code, sector_start_adress[ TARGET_SECTOR ], 1024, r );
    //printf("\n\n\n\n\n");
    
    blinkfunction = (function) (0xE000 | 1);

    //printf("\n\n\n\n\n");
    //print_function((char *)&foo , 100);
    
   // //printf("copied function \n");
		
		r = 0;
		r = blink();
		printf("The return value from blink is 0x%X\n\r", r);
		
		printf("The address of blink10times is 0x%X\n\r", &blink10times);
    
		r = blinkfunction();
		printf("The return value from blinkfunction is %x\n\r", r);
    while(1) {
       

    }
}



