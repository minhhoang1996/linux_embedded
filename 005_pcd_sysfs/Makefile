obj-m := pcd_sysfs.o
pcd_sysfs-objs += pcd_platform_driver_sysfs.o pcd_syscalls.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KERNEL_DIR=/source/linux_bbb_5.4/
HOST_KERNEL_DIR=/lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) clean

host:
	make -C $(HOST_KERNEL_DIR) M=$(PWD) modules
