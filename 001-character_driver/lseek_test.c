#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>

#define CHAR_DEVICE "/dev/eep-dev-0"
int main(int argc, char *argv[])
{
	int fd=0;
	char buf[20];
	printf("Start lseek test\n");
	fd = open("/dev/eep-dev-1", O_RDWR);
	if(fd<0){
		printf("Can not open device\n");
		return 1;
	}

	/*Read 20 byte*/
	if(read(fd, buf, 20) != 20){
		printf("Cannot read device\n");
		return 1;
	}

	printf("Read from device: %s\n", buf);

	/*Move the cursor to 10 times, relative to is actual position*/
	if(lseek(fd, 10, SEEK_CUR) < 0){
		printf("Cannot lseek device\n");
		return 1;
	}


	if(read(fd, buf, 20) != 20){
		printf("Cannot read device\n");
		return 1;
	}

	printf("Read from device: %s\n", buf);
	
	/*Move the cursor 10 time from the beginning of the file*/
	if(lseek(fd, 7, SEEK_SET) < 0){
		return 0;
	}

	if(read(fd, buf, 20) != 20){
		return 1;
	}

	printf("Read from device: %s\n", buf);
	
	close(fd);

	return 0;
}
