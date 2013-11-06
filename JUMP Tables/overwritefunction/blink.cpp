#include "jumptable.h"
#include "mbed.h"

unsigned int blink() {
	
	uint16_t addrh, addrl, addrl2;
	addrh = 0x1000;
	addrl = 0x5000;
	addrl2 = 0x6000;
	
	struct Jump_Table_Function *fnTable;
	struct Jump_Table_Data			*dataTable;
	fnTable = (struct Jump_Table_Function *)((addrh << 16) | addrl);
	dataTable = (struct Jump_Table_Data *) ((addrh << 16) | addrl2);
	////printf("The address of fn tables is 0x%X\n\r", fnTable);
	
	int i;
	for(i = 0; i < 5; i++) {
		//*(dataTable->led) = 1;
		fnTable->printf("LED set \n\r");
		fnTable->wait(0.5);
		
		//*(dataTable->led) = 0;
		fnTable->printf("LED set \n\r");
		fnTable->wait(0.5);
	}
	
	fnTable->printf("finished blinking leds\n\r");
	
	fnTable->blinky();

	
	return 1;
	
}