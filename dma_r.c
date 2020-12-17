//This file describes how data is transferred from FPGA to 
//processing system. The basic idea of how to control DMA can be found at
// Lauri's blog (http://lauri.xn--vsandi-pxa.com/hdl/zynq/xilinx-dma.html)
//The dma_receive() method will transfer the data from FPGA to physcical address handle2
//The datasheet of DMA can be found at
//http://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_from/to_user */
#include <linux/dma-mapping.h>
#include <linux/highmem.h>


#include <linux/device.h> 
#include <asm/io.h>

#include <linux/time.h>

#define MM2S_DMACR  0x00
#define MM2S_DMASR  0x04
#define MM2S_SA 0x18
#define MM2S_LENGTH 0x28

#define S2MM_DMACR  0x30
#define S2MM_DMASR  0x34
#define S2MM_DA 0x48
#define S2MM_LENGTH 0x58

#define DMA_PHY_ADDR 0x40400000
#define TRANSFER_LEN 1536 // bytes

#define TIME_OUT 30

// MODULE_LICENSE ("Dual BSD/GPL");

static dma_addr_t  handle2;

unsigned long* dma_virtual_address = NULL; // DMA module's virtual address
// char* virtual_source_address = NULL;
char* virtual_destination_address = NULL;

int dma_open (struct inode *inode, struct file *filp);
int dma_release (struct inode *inode, struct file *filp);
ssize_t dma_read (struct file *filp, char *buf, size_t count,
		     loff_t * f_pos);
ssize_t dma_write (struct file *filp, char *buf, size_t count,
		      loff_t * f_pos);
void dma_exit (void);
int dma_init (void);

extern int sendskb(unsigned char* dmadata, int dmadata_len);
extern int netmod_init(void);
// @@ Dengwang: dma2eth function

struct file_operations dma_fops = 
{
  .read = dma_read,
  .write = dma_write,
  .open = dma_open,
  .release = dma_release
};

// module_init (dma_init);
// module_exit (dma_exit);

int dma_major = 40;
char *dma_buffer;
// char *dma_buffer1;
char *dma_buffer2;

struct timespec t_begin, t_end;


void dma_set(unsigned long* dma_virtual_address, int offset, unsigned int value) {
    //iowrite32(value, dma_virtual_address + offset);
    dma_virtual_address[offset>>2] = value;
}

unsigned int dma_get(unsigned long* dma_virtual_address, int offset) {
    //return ioread32(dma_virtual_address + offset);
    return dma_virtual_address[offset>>2];
}

void dma_s2mm_status(unsigned long* dma_virtual_address) {
    //unsigned int status = ioread32(dma_virtual_address + S2MM_DMASR);
    unsigned int status = dma_get(dma_virtual_address, S2MM_DMASR);
    printk("Stream to memory-mapped status (0x%08x@0x%02x):", status, S2MM_DMASR);
    if (status & 0x00000001) printk(" halted"); else printk(" running");
    if (status & 0x00000002) printk(" idle");
    if (status & 0x00000008) printk(" SGIncld");
    if (status & 0x00000010) printk(" DMAIntErr");
    if (status & 0x00000020) printk(" DMASlvErr");
    if (status & 0x00000040) printk(" DMADecErr");
    if (status & 0x00000100) printk(" SGIntErr");
    if (status & 0x00000200) printk(" SGSlvErr");
    if (status & 0x00000400) printk(" SGDecErr");
    if (status & 0x00001000) printk(" IOC_Irq");
    if (status & 0x00002000) printk(" Dly_Irq");
    if (status & 0x00004000) printk(" Err_Irq");
    printk("\n");
}

void dma_mm2s_status(unsigned long* dma_virtual_address) {
    //unsigned int status = ioread32(dma_virtual_address + MM2S_DMASR);
	unsigned int status = dma_get(dma_virtual_address, MM2S_DMASR);
    printk("Memory-mapped to stream status (0x%08x@0x%02x):", status, MM2S_DMASR);
    if (status & 0x00000001) printk(" halted"); else printk(" running");
    if (status & 0x00000002) printk(" idle");
    if (status & 0x00000008) printk(" SGIncld");
    if (status & 0x00000010) printk(" DMAIntErr");
    if (status & 0x00000020) printk(" DMASlvErr");
    if (status & 0x00000040) printk(" DMADecErr");
    if (status & 0x00000100) printk(" SGIntErr");
    if (status & 0x00000200) printk(" SGSlvErr");
    if (status & 0x00000400) printk(" SGDecErr");
    if (status & 0x00001000) printk(" IOC_Irq");
    if (status & 0x00002000) printk(" Dly_Irq");
    if (status & 0x00004000) printk(" Err_Irq");
    printk("\n");
}

int dma_mm2s_sync(unsigned long* dma_virtual_address) {
    unsigned int mm2s_status =  dma_get(dma_virtual_address, MM2S_DMASR);
    unsigned long cnt = 0;
    while( !(mm2s_status & 1<<1) ){
        mm2s_status =  dma_get(dma_virtual_address, MM2S_DMASR);
        ++cnt;
        if(cnt == 100000) break;
    }
    return 0;
}

int dma_s2mm_sync(unsigned long* dma_virtual_address) {
    unsigned int s2mm_status = dma_get(dma_virtual_address, S2MM_DMASR);
    // unsigned long cnt = 0;
    getnstimeofday(&t_begin);

    while(!(s2mm_status & 1<<1)){              // only wait for idle

        s2mm_status = dma_get(dma_virtual_address, S2MM_DMASR);
        //++cnt;
        getnstimeofday(&t_end);

        if (t_end.tv_nsec - t_begin.tv_nsec > 2000000) { 
			     return -1;
        }
    }
    return 0;
}

void memdump(void* virtual_address, int byte_count) {
    char *p = virtual_address;
    int offset;
    for (offset = 0; offset < byte_count; offset++) {
        printk("%02x", p[offset]);
        if (offset % 4 == 3) { printk(" "); }
    }
    printk("\n");
}

void checkData(char *bufferIn, char *bufferOut, unsigned int elems){
	if(!memcmp(bufferIn, bufferOut, elems*sizeof(char))){
		printk("DMA Ok!\n");
	}
	else{
    printk("NOT Ok!\n");
  }
}

int
dma_init (void)
{
  int result;
  result = register_chrdev (dma_major, "dma", &dma_fops);
  if (result < 0)
    {
      printk ("<1>dma: cannot obtain major number %d\n", dma_major);
      return result;
    }

  dma_buffer2 = dma_alloc_coherent(NULL, TRANSFER_LEN, &handle2, GFP_KERNEL);//use dma_alloc-coherent(). Don't use kmalloc(), since it will cache data between virtual addr and physical addr
  if (!dma_buffer2)
  {
    printk("<1>kmalloc buffer1 error\n");
    result = -ENOMEM;
    goto fail;
  }
  memset (dma_buffer2, 0, TRANSFER_LEN);


  if(!request_mem_region(DMA_PHY_ADDR, 0xA0, "nihao")){
  	printk("<1> dma_virtual_address request error\n");
	result = -ENOMEM;		
	goto fail;
  }
  dma_virtual_address = (unsigned long *)ioremap(DMA_PHY_ADDR, 0xA0);//To get Custom IP--DMA moudle's virtual address
  if(dma_virtual_address == NULL){
  	printk("<1> dma_virtual_address map error\n");
	result = -ENOMEM;		
	goto fail;
  }

  printk("<1> physcical_destination_address (handle2) is %lx\n", handle2);
  virtual_destination_address = dma_buffer2;


  printk("Destination memory block: "); memdump(virtual_destination_address, TRANSFER_LEN);

  dma_s2mm_status(dma_virtual_address);
  printk("Resetting s2mm DMA\n");
  dma_set(dma_virtual_address, S2MM_DMACR, 4);  //S2MM_DMACR
  dma_s2mm_status(dma_virtual_address);
  printk("Resetting DMA over\n");

  printk("Halting s2mm DMA\n");
  dma_set(dma_virtual_address, S2MM_DMACR, 0);  //S2MM_DMACR
  dma_s2mm_status(dma_virtual_address);

  printk("Writing s2mm destination address\n");
  dma_set(dma_virtual_address, S2MM_DA, handle2); // Write destination address
  dma_s2mm_status(dma_virtual_address);

  printk("Starting S2MM channel with all interrupts unmasked...\n");
  dma_set(dma_virtual_address, S2MM_DMACR, 0x0001);
  dma_s2mm_status(dma_virtual_address);  

  printk ("<1> Init network module\n");
  netmod_init();

  printk ("<1> Inserting module v1.0 comment on comment\n");

  return 0;

fail:
  dma_exit ();
  return result;
}

void
dma_exit (void)
{
  unregister_chrdev (dma_major, "dma");
  if(dma_buffer2)
  {
    // kfree (dma_buffer1);
    dma_free_coherent(NULL, TRANSFER_LEN, dma_buffer2, handle2);
  }
	if (dma_virtual_address){
		iounmap(dma_virtual_address);
		release_mem_region(DMA_PHY_ADDR, 0xA0);
	}


  printk ("<1>Removing memory module\n");
}

int
dma_open (struct inode *inode, struct file *filp)
{
  return 0;
}

int
dma_release (struct inode *inode, struct file *filp)
{
  return 0;
}

ssize_t
dma_read (struct file * filp, char *buf, size_t count, loff_t * f_pos)
{
  return 0;
}

int dma_receive(int to_eth){
      unsigned int bytes;

      if(to_eth){

	        //printk("Writing S2MM transfer length...\n");
	        dma_set(dma_virtual_address, S2MM_LENGTH, TRANSFER_LEN);
	        //dma_s2mm_status(dma_virtual_address);

	        //printk("Waiting for S2MM sychronization...\n");
	        if(dma_s2mm_sync(dma_virtual_address) == -1 ){
	          printk("s2mm break\n");

	          return 2; // If this locks up make sure all memory ranges are assigned under Address Editor!
	        }

	        bytes = dma_get(dma_virtual_address, S2MM_LENGTH);
	        //printk("Actual bytes received is: %d\n", bytes);
	        //printk("@@ Destination memory block: "); memdump(virtual_destination_address, bytes);
	        
	        sendskb(virtual_destination_address, bytes);

      }
      return 0;
}


ssize_t
dma_write (struct file * filp, char *buf, size_t count, loff_t * f_pos)
{
  return 0;
}