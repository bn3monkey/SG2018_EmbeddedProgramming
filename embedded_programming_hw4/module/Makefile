obj-m = dev_driver.o
#module-obj := 
CC := arm-none-linux-gnueabi-gcc


KDIR := /work/achroimx_kernel
PWD := $(shell pwd)

driver :
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean :
	rm -rf *.o *.ko *.mod.c *.order *.symvers
	rm -rf .*.ko.cmd
	rm -rf .*.mod.o.cmd
	rm -rf .*.o.cmd
