#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define NAME "Hoang1"

#define NUM 3
#define SIZE (NUM*sizeof(int))

int main()
{
	int fd = shm_open(NAME, O_RDWR , 0666);
	if(fd<0){
		return -1;
	}
	
	int *data = (int*)mmap(0,SIZE,PROT_READ|PROT_WRITE, MAP_SHARED,fd,0);
	printf("%p\n", data);
	
	for (int i=0; i<NUM;i++)
	{
		printf("%d \n", data[i]);
	}

	
	
	munmap(data,SIZE);
	close(fd);
	shm_unlink(NAME);
	
	return 0;
}