#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <unistd.h>

#define GPIO_ADDR_BASE  0x4804C000
#define ADDR_SIZE       (0x1000)
#define GPIO_SETDATAOUT_OFFSET          0x194
#define GPIO_CLEARDATAOUT_OFFSET        0x190
#define GPIO_OE_OFFSET                  0x134
#define LED                             ~(1 << 14)
#define DATA_OUT			(1 << 14)

int main(){
	int pagesize= getpagesize();
	off_t phys_addr = GPIO_ADDR_BASE;
	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	void *map_base = mmap(NULL,
				pagesize,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				mem_fd,
				phys_addr);
	
	
	
	*(volatile uint32_t *)(map_base + GPIO_OE_OFFSET) &= LED;
	
	while(1){
		*(volatile uint32_t *)(map_base + GPIO_SETDATAOUT_OFFSET) = DATA_OUT;
		sleep(1);
		*(volatile uint32_t *)(map_base + GPIO_CLEARDATAOUT_OFFSET) = DATA_OUT;
		sleep(1);
	}
	
	
	

	munmap(map_base, pagesize);
	return 0;
}

