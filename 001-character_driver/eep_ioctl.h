#include <linux/ioctl.h>

#define EEP_MAGIC 'R'
#define ERASE_BUF 0x01
#define WRITE_BUF 0x02
#define READ_BUF  0x03

struct ioctl_data{
	char ioctl_buff[30];
	int val;
};
/*define our ioctl numbers*/

#define EEP_ERASE	_IO(EEP_MAGIC, ERASE_BUF)
#define EEP_WRITE_BUF 	_IOW(EEP_MAGIC, WRITE_BUF, struct ioctl_data)
#define EEP_READ_BUF 	_IOR(EEP_MAGIC, READ_BUF, struct ioctl_data)
