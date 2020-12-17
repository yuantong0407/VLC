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
#include <linux/time.h>
#include <asm/io.h>

#define MM2S_DMACR  0x00
#define MM2S_DMASR  0x04
#define MM2S_SA 0x18
#define MM2S_LENGTH 0x28

#define S2MM_DMACR  0x30
#define S2MM_DMASR  0x34
#define S2MM_DA 0x48
#define S2MM_LENGTH 0x58

#define DMA_PHY_ADDR 0x40400000
#define TRANSFER_LEN 1536//  bytes

static dma_addr_t handle1;

unsigned long* dma_virtual_address = NULL; // DMA module's virtual address
char* virtual_source_address = NULL;
// char* virtual_destination_address = NULL;

// int dma_open (struct inode *inode, struct file *filp);
// int dma_release (struct inode *inode, struct file *filp);
// ssize_t dma_read (struct file *filp, char *buf, size_t count,
// 		     loff_t * f_pos);
// ssize_t dma_write (struct file *filp, char *buf, size_t count,
// 		      loff_t * f_pos);
// void dma_exit (void);
// int dma_init (void);

int dma_major = 40;
char *dma_buffer;
char *dma_buffer1;
char *dma_buffer2;


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
    while(!(mm2s_status & 1<<1) ){              // only wait for idle
        // dma_s2mm_status(dma_virtual_address);
        // dma_mm2s_status(dma_virtual_address);

        mm2s_status =  dma_get(dma_virtual_address, MM2S_DMASR);
        ++cnt;
        if(cnt == 200000000) {
          printk("break mm2s sync\n");
          break;
        }
    }
    return 0;
}

int dma_s2mm_sync(unsigned long* dma_virtual_address) {
    unsigned int s2mm_status = dma_get(dma_virtual_address, S2MM_DMASR);
    unsigned long cnt = 0;
    while( !(s2mm_status & 1<<1)){
        // dma_s2mm_status(dma_virtual_address);
        // dma_mm2s_status(dma_virtual_address);

        s2mm_status = dma_get(dma_virtual_address, S2MM_DMASR);
        ++cnt;
        if(cnt == 10000000) break;
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
	int i;

	if(!memcmp(bufferIn, bufferOut, elems*sizeof(char))){
		printk("DMA Ok!\n");
	}
	else{
		for(i=0;i<elems;i++)
			printk("%d\t%d\t%d\t%d\n", i, bufferIn[i], bufferOut[i], (i==0 ? 0 : bufferOut[i] - bufferOut[i-1]));
	}
}

void
dma_exit (void);

int
dma_init (void)
{
  int result;
  // int i;

  /* Allocating memory for the buffer */
  // dma_buffer1 = kmalloc (TRANSFER_LEN, GFP_KERNEL);
  dma_buffer1 = dma_alloc_coherent(NULL, TRANSFER_LEN, &handle1, GFP_KERNEL);
  if (!dma_buffer1)
  {
    printk("<1>kmalloc buffer1 error\n");
    result = -ENOMEM;
    goto fail;
  }
  memset (dma_buffer1, 0, TRANSFER_LEN);


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

  // physcical_source_address = virt_to_phys(dma_buffer1);
  printk("<1> physcical_source_address (handle1) is %lx\n", handle1);
  virtual_source_address = dma_buffer1;


// static u8 DMASendBuffer[64]={0xBC,0x5F,0xF4,0x9A,0x3B,0x63,0xC8,0x60,0x00,0xC5,0x05,0xEA,0x08,0x00,0x45,0x00,0x05,0x4C,
//       0x54,0xAF,0x00,0x00,0x40,0x11,0x9D,0x88,0xC0,0xA8,0x01,0x0F,0xC0,0xA8,0x01,0x0A,0xFA,0x57,0x13,0x8C,0x05,0x38,
//       0xD7,0xEC,0x80,0xA1,0xE6,0x18,0x44,0xB0,0xF0,0xF8,0xAF,0xCE,0xEC,0xA6,0x47,0x00,0x5D,0x15,0x6D,0xF4,0xA4,0x4D,0x60,0x9B};


  printk("Souce memory block:      ");  memdump(virtual_source_address, TRANSFER_LEN);
  // printk("Destination memory block: "); memdump(virtual_destination_address, TRANSFER_LEN);

  dma_mm2s_status(dma_virtual_address);
  printk("Resetting mm2s DMA\n");
  dma_set(dma_virtual_address, MM2S_DMACR, 4);  //MM2S_DMACR
  dma_mm2s_status(dma_virtual_address);  
  printk("Resetting mm2s DMA over\n");

  printk("Halting mm2s DMA\n");
  dma_set(dma_virtual_address, MM2S_DMACR, 0);  //MM2S_DMACR
  dma_mm2s_status(dma_virtual_address);

  printk("Writing mm2s source address...\n");
  dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
  dma_mm2s_status(dma_virtual_address);

  printk("Starting MM2S channel with all interrupts unmasked...\n");
  dma_set(dma_virtual_address, MM2S_DMACR, 0x0001);
  dma_mm2s_status(dma_virtual_address);

  printk ("<1> dma init complete\n");
  return 0;

fail:
  dma_exit ();
  return result;
}

void
dma_exit (void)
{
  // unregister_chrdev (dma_major, "dma");
  if(dma_buffer1)
  {
    // kfree (dma_buffer1);
    dma_free_coherent(NULL, TRANSFER_LEN, dma_buffer1, handle1);
  }

	if (dma_virtual_address){
		iounmap(dma_virtual_address);
		release_mem_region(DMA_PHY_ADDR, 0xA0);
	}
	// if (virtual_source_address){
	// 	iounmap(virtual_source_address);
	// 	release_mem_region(0x0e000000, 0xA0);
	// }
	// if (virtual_destination_address){
	// 	iounmap(virtual_destination_address);
	// 	release_mem_region(0x0f000000, 0xA0);
	// }	

  printk ("<1>dma exit\n");
}

// int
// dma_open (struct inode *inode, struct file *filp)
// {
//   return 0;
// }

// int
// dma_release (struct inode *inode, struct file *filp)
// {
//   return 0;
// }

// ssize_t
// dma_read (struct file * filp, char *buf, size_t count, loff_t * f_pos)
// {
//   /* Transfering data to user space */
//   copy_to_user (buf, dma_buffer, 1);
//   /* Changing reading position as best suits */
//   if (*f_pos == 0)
//     {
//       *f_pos += 1;
//       return 1;
//     }
//   else
//     {
//       return 0;
//     }
// }

void dma_write_simple(char* buff, unsigned int buff_len){
      memcpy(virtual_source_address, buff, buff_len);

      // printk("Souce memory block:      ");  memdump(virtual_source_address, buff_len);

      // printk("Writing mm2s source address...\n");
      dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
      // dma_mm2s_status(dma_virtual_address);

      // printk("Writing MM2S transfer length...\n");
      dma_set(dma_virtual_address, MM2S_LENGTH, buff_len);
      // dma_mm2s_status(dma_virtual_address);

      // printk("Waiting for MM2S synchronization...\n");
      dma_mm2s_sync(dma_virtual_address);

      // dma_mm2s_status(dma_virtual_address);

}


// ssize_t
// dma_write (struct file * filp, char *buf, size_t count, loff_t * f_pos)
// {
//   char* buffer = kmalloc (count, GFP_KERNEL);
//   int i;
//   // struct timespec t_begin, t_end;

//   // int cnt = 0;
//   copy_from_user (buffer, buf, count);
//   for(i = 0; i < count; ++i)
//   	printk("%c", buffer[i]);
//   printk("\n");

//   if(buffer[count-1] == '1'){
//     // printk("Resetting mm2s DMA\n");
//     // dma_set(dma_virtual_address, MM2S_DMACR, 4);  //MM2S_DMACR
//     // dma_mm2s_status(dma_virtual_address);  
//     // printk("Resetting mm2s DMA over\n");
//     for(i = 0; i < 10; ++i){
//     virtual_source_address[0] = 0xBC;
//     virtual_source_address[1] = 0x5F;
//     virtual_source_address[2] = 0xF4;
//     virtual_source_address[3] = 0x9A;
//     virtual_source_address[4] = 0x3B;
//     virtual_source_address[5] = 0x63;
//       virtual_source_address[6] = i;
//       virtual_source_address[7] = i;
//       virtual_source_address[8] = i;
//       virtual_source_address[9] = i;
//     // dcache_clean();
//     // flush_dcache_page((void*)virtual_source_address);

//       // while(cnt < 100000000)
//       //   cnt++;
//       printk("Souce memory block:      ");  memdump(virtual_source_address, TRANSFER_LEN);

//       printk("Writing mm2s source address...\n");
//       dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
//       dma_mm2s_status(dma_virtual_address);

//       printk("Writing MM2S transfer length...\n");
//       dma_set(dma_virtual_address, MM2S_LENGTH, TRANSFER_LEN);
//       dma_mm2s_status(dma_virtual_address);

//       printk("Waiting for MM2S synchronization...\n");
//       dma_mm2s_sync(dma_virtual_address);

//       dma_mm2s_status(dma_virtual_address);
//       }

//   }
//   else if(buffer[count-1] == '2'){
//     // printk("Resetting mm2s DMA\n");
//     // dma_set(dma_virtual_address, MM2S_DMACR, 4);  //MM2S_DMACR
//     // dma_mm2s_status(dma_virtual_address);  
//     // printk("Resetting mm2s DMA over\n");


//     virtual_source_address[0] = 0xBC;  
//     virtual_source_address[1] = 0x5F;
//     virtual_source_address[2] = 0xF4;
//     virtual_source_address[3] = 0x9A;
//     virtual_source_address[4] = 0x3B;
//     virtual_source_address[5] = 0x63;    
//       virtual_source_address[6] = 0x06;
//       virtual_source_address[7] = 0x07;
//       virtual_source_address[8] = 0x08;
//       virtual_source_address[9] = 0x09;
//     // flush_dcache_page((void*)virtual_source_address);

//       // while(cnt < 100000000)
//       //   cnt++;
//       printk("Souce memory block:      ");  memdump(virtual_source_address, TRANSFER_LEN);

//       printk("Writing mm2s source address...\n");
//       dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
//       dma_mm2s_status(dma_virtual_address);

//       printk("Writing MM2S transfer length...\n");
//       dma_set(dma_virtual_address, MM2S_LENGTH, TRANSFER_LEN);
//       dma_mm2s_status(dma_virtual_address);

//       printk("Waiting for MM2S synchronization...\n");
//       dma_mm2s_sync(dma_virtual_address);

//       dma_mm2s_status(dma_virtual_address);
//   }
//   else if(buffer[count-1] == '3'){
//     // printk("Resetting mm2s DMA\n");
//     // dma_set(dma_virtual_address, MM2S_DMACR, 4);  //MM2S_DMACR
//     // dma_mm2s_status(dma_virtual_address);  
//     // printk("Resetting mm2s DMA over\n");

//     virtual_source_address[0] = 0xBC;  
//     virtual_source_address[1] = 0x5F;
//     virtual_source_address[2] = 0xF4;
//     virtual_source_address[3] = 0x9A;
//     virtual_source_address[4] = 0x3B;
//     virtual_source_address[5] = 0x63;    
//       virtual_source_address[6] = 0x16;
//       virtual_source_address[9] = 0x19;
//     // flush_dcache_page((void*)virtual_source_address);

//       // while(cnt < 100000000)
//       //   cnt++;
//       printk("Souce memory block:      ");  memdump(virtual_source_address, TRANSFER_LEN);

//       printk("Writing mm2s source address...\n");
//       dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
//       dma_mm2s_status(dma_virtual_address);

//       printk("Writing MM2S transfer length...\n");
//       dma_set(dma_virtual_address, MM2S_LENGTH, TRANSFER_LEN);
//       dma_mm2s_status(dma_virtual_address);

//       printk("Waiting for MM2S synchronization...\n");
//       dma_mm2s_sync(dma_virtual_address);

//       dma_mm2s_status(dma_virtual_address);
//   }
//   else if(buffer[count-1] == 'r'){ // reset
//     printk("Resetting mm2s DMA\n");
//     dma_set(dma_virtual_address, MM2S_DMACR, 4);  //MM2S_DMACR
//     dma_mm2s_status(dma_virtual_address);    
//     printk("Resetting mm2s DMA over\n");    
//   }
//   else if(buffer[count-1] == 's'){ // set start/run bit
//     printk("Writing mm2s source address...\n");
//     dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
//     dma_mm2s_status(dma_virtual_address);

//     printk("Starting MM2S channel with all interrupts unmasked...\n");
//     dma_set(dma_virtual_address, MM2S_DMACR, 0x0001);
//     dma_mm2s_status(dma_virtual_address);
//   }
//   else{
//   	printk("buffer[count-1] %c\n", buffer[count-1]);
//   }

//   return 1;
// }