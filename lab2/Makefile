obj-m += lab2.o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
install:
	sudo insmod lab2.ko
remove:
	sudo rmmod lab2
rerun:
	make remove && make clean && make all && sudo insmod lab2.ko
fdisk: 
	sudo fdisk -l /dev/mydisk
