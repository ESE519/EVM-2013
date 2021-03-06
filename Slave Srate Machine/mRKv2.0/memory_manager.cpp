#include "memory_manager.h"

unsigned int current_sector;
uint32_t  current_address;

uint32_t  start_address, end_address;
IAP iap;


int set_start_address(uint32_t address) {
	uint32_t temp;
	
	if(address < 0xFFFF) {
		if((address & 0xFFF) == 0) {        //it is aligned on sector start address.
			start_address = address;
			temp = address & 0xF000;
			temp = temp >> 12;
			current_sector = temp;
			current_address = start_address;
			iap.prepare(current_sector, current_sector);
			return 0;
		}
		
		else
			return -1;
			
	}
	
	else {
		
		if(((address & 0xFFFF) == 0) || ((address & 0xFFFF) == 0x8000)) {
			start_address = address;
			temp = address & 0xF0000;
			temp = temp >> 16;
			current_sector = 16 + 2 * (temp - 1);
			
			if(address & 0xF000)
				current_sector++;
				
			current_address = start_address;
			iap.prepare(current_sector, current_sector);
			return 0;
		}
		
		else
			return -1;
	}
}
			



			
uint32_t get_start_address() {
	return start_address;
}



int set_end_address(uint32_t address) {
	
	if(address > start_address) {
		end_address = address;
		return 0;
	}
	
	else
		return -1;
}




uint32_t get_end_address() {
	return end_address;
}	






char * copy_code_flash(char *code, unsigned int size, int *errno) {
	unsigned int rem;
	int r;
	
	if(size > 4096) {
		*errno = -1;
		return NULL;
	}
		
	if( (size % 256) != 0) {
		rem = size >> 8;
		size = (rem + 1) << 8;
	}
	
	if((current_address + size) > end_address) {
		*errno = -1;
		return NULL;
	}
		
	//check if the writing here crosses sector boundary
	if( (current_address + size) >= (unsigned int)sector_start_adress[current_sector + 1] ) {
		//prepare next sector	
		current_sector++;
		current_address = (unsigned int) sector_start_adress[current_sector];

	
	}
	
	r = iap.prepare(current_sector, current_sector);
		
	if(r != 0) {
		*errno = r;
		return NULL;
	}
	
	
	r = iap.write(code, (char *) current_address, size);
	
	
	if(r == 0) {
		current_address = current_address + size;
	}
	
	*errno = r;
	if(r) {
		return NULL;
	}
	else{
		return (char *)(current_address - size);
	}
}
	



/*int main() {
	int r;
	
	r = set_start_address(0x1000);
	if(r == 0)
		printf("First test passed\n");
	else
		printf("First test failed\n");
		
	
	if((current_sector != 1) || (current_address != 0x1000))
		printf("Second test failed\n");
		
	else 
		printf("second test passed\n");
		
	
	r = set_start_address(0xFFF2);
	if(r == 0)
		printf("Third test failed\n");
	else
		printf("Third test passed\n");
		
	
	r = set_start_address(0x1F000);
	if(r == 0)
		printf("Fourth test failed\n");
	else
		printf("Fourth test passed\n");
		
	r = set_start_address(0x58000);
	if(r == 0)
		printf("Fifth test passed\n");
	else
		printf("Fifth test failed\n");
		
	if((current_sector != 25) || (current_address != 0x70000)) {
		printf("sixth test failed\n");
		printf("sector: %d\t address: %X\n", current_sector, current_address);
	}
	else
		printf("sixth test passed\n");
		
}*/

		
