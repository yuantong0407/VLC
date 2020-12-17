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

#define QSIZE 512
static dma_addr_t handle[QSIZE];
char* virtual_source_address[QSIZE];

unsigned long* dma_virtual_address = NULL; // DMA module's virtual address
// char* virtual_source_address = NULL;
// char* virtual_destination_address = NULL;
extern void fill_snw_queue(char* v, dma_addr_t h, int i);

int dma_major = 40;
char *dma_buffer;
char *dma_buffer1;

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
    getnstimeofday(&t_begin);

    while(!(mm2s_status & 1<<1) ){              // only wait for idle
        // dma_s2mm_status(dma_virtual_address);
        // dma_mm2s_status(dma_virtual_address);

        mm2s_status =  dma_get(dma_virtual_address, MM2S_DMASR);


        getnstimeofday(&t_end);

        if (t_end.tv_nsec - t_begin.tv_nsec > 2000000) {
           return -1;
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
  int i;

  /* Allocating memory for the buffer */
  // dma_buffer1 = kmalloc (TRANSFER_LEN, GFP_KERNEL);
  for(i = 0; i < QSIZE; ++i){
    virtual_source_address[i] = dma_alloc_coherent(NULL, TRANSFER_LEN, &handle[i], GFP_KERNEL);
    if (!virtual_source_address[i])
    {
      printk("<1>kmalloc virtual_source_address error\n");
      result = -ENOMEM;
      goto fail;
    }
    fill_snw_queue(virtual_source_address[i], handle[i], i);
    memset(virtual_source_address[i], 0, TRANSFER_LEN);
  }
  // memset (dma_buffer1, 0, TRANSFER_LEN);


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
  // for(i = 0; i < QSIZE; ++i){
  //   printk("<1> physcical_source_address (handle[i]) is %lx\n", handle[i]);
  // }
  // virtual_source_address = dma_buffer1;


  // printk("Souce memory block:      ");  memdump(virtual_source_address, TRANSFER_LEN);
  // printk("Destination memory block: "); memdump(virtual_destination_address, TRANSFER_LEN);

  dma_mm2s_status(dma_virtual_address);
  printk("Resetting mm2s DMA\n");
  dma_set(dma_virtual_address, MM2S_DMACR, 4);  //MM2S_DMACR
  dma_mm2s_status(dma_virtual_address);  
  printk("Resetting mm2s DMA over\n");

  printk("Halting mm2s DMA\n");
  dma_set(dma_virtual_address, MM2S_DMACR, 0);  //MM2S_DMACR
  dma_mm2s_status(dma_virtual_address);

  // printk("Writing mm2s source address...\n");
  // dma_set(dma_virtual_address, MM2S_SA, handle1); // Write source address
  // dma_mm2s_status(dma_virtual_address);

  printk("Starting MM2S channel with all interrupts unmasked...\n");
  dma_set(dma_virtual_address, MM2S_DMACR, 0x0001);
  dma_mm2s_status(dma_virtual_address);

  printk ("<1> dma QSIZE init complete\n");
  return 0;

fail:
  dma_exit ();
  return result;
}

void
dma_exit (void)
{
  // unregister_chrdev (dma_major, "dma");
  int i;
  for(i = 0; i < QSIZE; ++i){
    if(virtual_source_address[i])
    {
      // kfree (dma_buffer1);
      dma_free_coherent(NULL, TRANSFER_LEN, virtual_source_address[i], 
                                            handle[i]);
    }
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

int dma_write_simple(char* buff, dma_addr_t handle, unsigned int buff_len){
      // memcpy(virtual_source_address, buff, buff_len);
      // kfree(buff);

      // printk("Souce memory block:      ");  memdump(virtual_source_address, buff_len);

      printk("Writing mm2s source address...\n");
      dma_set(dma_virtual_address, MM2S_SA, handle); // Write source address
      dma_mm2s_status(dma_virtual_address);

      // printk("Writing MM2S transfer length...\n");
      dma_set(dma_virtual_address, MM2S_LENGTH, buff_len);
      // dma_mm2s_status(dma_virtual_address);

      // printk("Waiting for MM2S synchronization...\n");
      if(dma_mm2s_sync(dma_virtual_address) == -1){
        printk("break mm2s sync\n");
        return 1;
      }

      return 0;
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