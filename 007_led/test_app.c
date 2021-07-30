#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
 
int main(int argc, char **argv)
{
        int fd;
        int32_t value, number;
        printf("*********************************\n");
		if(argc < 2){
			printf("The testing application should be run with second argument as <led device>\n");
			return -1;
		}
        printf("\nOpening Driver\n");
        fd = open(argv[1], O_RDWR);
        if(fd < 0) {
                printf("Cannot open led device ...\n");
                return 0;
        }
 
        printf("Enter the timeout(milliseconds) to send\n");
        scanf("%d",&number);
        printf("Writing timeout(milliseconds) to Driver\n");
        ioctl(fd, WR_VALUE, (int32_t*) &number); 
 
        printf("Reading timeout(milliseconds) from Driver\n");
        ioctl(fd, RD_VALUE, (int32_t*) &value);
        printf("Timeout(milliseconds) is %d\n", value);
		printf("Frequency(Hz) is %3fHz\n", (1*1000)/(2*(float)value));
		
        printf("Closing Driver\n");
        close(fd);
}