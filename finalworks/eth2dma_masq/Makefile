ifneq ($(KERNELRELEASE),)  
	obj-m := oureth2dma_masq.o
	oureth2dma_masq-objs := eth2dma.o dma_w.o  
else  
	KERNEL_DIR := /home/dwtang/linux-digilent/linux-digilent  
	PWD := $(shell pwd) 
all:  
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules ARCH=arm  
clean:  
	rm *.o *.ko *.mod.c
endif  
