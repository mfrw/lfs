obj-m := learnfs.o
learnfs-objs := lfs.o

all: ko mkfs-lfs

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mkfs-lfs_SOURCES:
	mkfs-lfs simple.h
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f mkfs-lfs
