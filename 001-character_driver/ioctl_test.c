#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "eep_ioctl.h"


int main(){
	int fd;
	
	/*
	char user_val[1024];
       	char get_val[1024];
	*/
	struct ioctl_data user_data = {
		.ioctl_buff = "Nguyen Minh Hoang",
		.val = 9
	};
	struct ioctl_data get_data;

	fd = open("/dev/eep-dev-0", O_RDWR);
	if(fd == -1){
		printf("Error while opening the eep device\n");
		return -1;
	}
	

	if( ioctl(fd, EEP_WRITE_BUF, &user_data) )
	{	
		return -1;
	}
	if( ioctl(fd, EEP_READ_BUF, &get_data) ){
		return -1;
	}

	printf("Ioctl reads from device the buffer: %s\n", get_data.ioctl_buff);
	printf("Ioctl reads form device the value: %d\n", get_data.val);

	close(fd);
	return 0;
}
