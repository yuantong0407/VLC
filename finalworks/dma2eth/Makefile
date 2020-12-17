ifneq ($(KERNELRELEASE),)
	obj-m := ourdma2eth.o
	ourdma2eth-objs := dma_r.o  dma2eth.o
else
	KERNEL_DIR := /home/dwtang/linux-digilent/linux-digilent  
	PWD := $(shell pwd)  
all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules ARCH=arm  
clean:
	rm *.o *.ko *.mod.c
endif