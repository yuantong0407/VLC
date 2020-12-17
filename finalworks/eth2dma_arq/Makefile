ifneq ($(KERNELRELEASE),)  
	obj-m := oureth2dma_arq.o
	oureth2dma_arq-objs := arq.o  dma_w.o eth2dma.o 
else  
	KERNEL_DIR := /home/dwtang/linux-digilent/linux-digilent  
	PWD := $(shell pwd) 
all:  
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules ARCH=arm  
clean:  
	rm *.o *.ko *.mod.c
endif  
