#include <asm/io.h>
#include <asm/unaligned.h>
#include <linux/delay.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/netpoll.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/udp.h>
#include <net/addrconf.h>

MODULE_LICENSE("GPL");

#define DEVICE_NAME0 "ACKIRQ"
#define DEVICE_NAME1 "NAKIRQ"

#define IR0  62
#define IR1  63

#define INADDR_RX ((unsigned long int) 0xc0a80202U) //192.168.2.2
#define INADDR_TX  ((unsigned long int) 0xc0a80201U) //192.168.2.1
// #define INADDR_TX  ((unsigned long int) 0xc0a80203U) //192.168.2.3 // for test

const unsigned char RX_MAC[ETH_ALEN] = {0xEC, 0x08, 0x6B, 0x1F, 0x96, 0x99}; // rx
const unsigned char TX_MAC[ETH_ALEN] = {0x3C, 0x46, 0xD8, 0x93, 0x77, 0x3B}; // tx
// const unsigned char HER_MAC[ETH_ALEN] = {0xE8, 0x03, 0x9A, 0xFB, 0x01, 0x72}; // Samsung WiFi

extern int irq_set_irq_type(unsigned int irq, unsigned int type);
extern int dma_receive(int to_eth);
extern int dma_init (void);
extern void dma_exit (void);
static struct platform_device *pdev0;
static struct platform_device *pdev1;
//void *dev_virtaddr;
//static int int_cnt;

static struct net_device* dev;
static struct sk_buff *skb;

static void net_init(void){
  dev = dev_get_by_name(&init_net, "wlan0");
}

static int sendsomething(const char* message, unsigned int len){
  int total_len, eth_len, ip_len, udp_len;
  struct iphdr *iph;
  struct udphdr *udph;
  struct ethhdr *eth;
  // char message[16] = "liusihangshigesb";
  // int len = 16;
  static atomic_t ip_ident;

  udp_len = len + sizeof(struct udphdr);
  ip_len = udp_len + sizeof(struct iphdr);
  eth_len = ip_len + sizeof(struct ethhdr);
  total_len = ip_len + LL_RESERVED_SPACE(dev);

  skb = alloc_skb(total_len + dev->needed_tailroom, GFP_ATOMIC);
  if (!skb){
    return 1;
  }
  atomic_set(&skb->users, 1);
  skb_reserve(skb, total_len - len);

  // printk("@@skb_reserve\n");  
  
  // skb_put(skb, len);
  // memcpy(skb->data, message, len);
  skb_copy_to_linear_data(skb, message, len);
    skb_put(skb, len);

  udph = (struct udphdr *) skb_push(skb, sizeof(struct udphdr));
  skb_reset_transport_header(skb);
  
  // printk("@@skb_push udphdr\n");  
  
  udph->source = htons(6665);
  udph->dest = htons(6666);
  udph->len = htons(udp_len);
  udph->check = csum_tcpudp_magic(htonl(INADDR_RX),
    htonl(INADDR_TX),
    udp_len, IPPROTO_UDP,
    csum_partial(udph, udp_len, 0));;

  iph = (struct iphdr *)skb_push(skb, sizeof(struct iphdr));
  skb_reset_network_header(skb);

  // printk("@@skb_push iphdr\n");
  
  put_unaligned(0x45, (unsigned char *)iph);
  // iph->version  = 4;
  // iph->ihl      = 5;
  iph->tos      = 0;
  put_unaligned(htons(ip_len), &(iph->tot_len));
  iph->id       = htons(atomic_inc_return(&ip_ident));
  // iph->id       = 0;
  iph->frag_off = 0;
  iph->ttl      = 64;
  iph->protocol = IPPROTO_UDP;
  iph->check    = 0;
  iph->saddr    = htonl(INADDR_RX);
  iph->daddr    = htonl(INADDR_TX);
  iph->check    = ip_fast_csum((unsigned char *)iph, iph->ihl);

  eth = (struct ethhdr *) skb_push(skb, ETH_HLEN);
  skb_reset_mac_header(skb);

  // printk("@@skb_push skb\n"); 
  
  eth->h_proto = htons(ETH_P_IP);
  memcpy(eth->h_source, RX_MAC, ETH_ALEN);
  memcpy(eth->h_dest, TX_MAC, ETH_ALEN);
  
  skb->protocol = eth->h_proto;
  // printk("@@%lu %lu\n", sizeof(*eth->h_source), sizeof(*eth->h_dest));
  
  skb->dev = dev;

  dev_queue_xmit(skb);

  printk("@@send skb\n"); 


  return 0; // you gonna fly high, you'll never gonna die, you'll gonna make it if you try
}

static irqreturn_t xilaxitimer_isr0(int irq,void*dev_id)        
{      
  char ackmessage[6];
  unsigned char seq = 1U;
  printk("ACK!\n");

  memcpy(ackmessage, "@@ACK", 5);
  memcpy(ackmessage+5, seq, 1);
  sendsomething(ackmessage, 6);

  printk("ready to receive to eth\n");
  // dma_receive(1);    // to_eth = 1

  return IRQ_HANDLED;
}

static irqreturn_t xilaxitimer_isr1(int irq,void*dev_id)        
{      
  char ackmessage[6];
  unsigned char seq = 1U;
  printk("NAK!\n"); 
  memcpy(ackmessage, "@@NAK", 5);
  memcpy(ackmessage+5, seq, 1);
  sendsomething(ackmessage, 6);

  printk("ready to receive\n");
  // dma_receive(1);    // to_eth = 1

  return IRQ_HANDLED;
}

static int __init ack_init(void)
{
  
  net_init();
  printk(KERN_INFO "xilaxitimer_init: Initialize Module\n");
  
  /* 
   * Register ISR
   */
  //IR0
  if (request_irq(IR0, xilaxitimer_isr0, 0, DEVICE_NAME0, NULL)) {
    printk(KERN_ERR "xilaxitimer_init: Cannot register IR0 %d\n", IR0);
    return -EIO;
  }
  else {
    printk(KERN_INFO "xilaxitimer_init: Registered IR0 %d\n", IR0);
  }
  irq_set_irq_type(IR0, IRQ_TYPE_EDGE_RISING);
  
  //IR1
  if (request_irq(IR1, xilaxitimer_isr1, 0, DEVICE_NAME1, NULL)) {
    printk(KERN_ERR "xilaxitimer_init: Cannot register IR1 %d\n", IR1);
    return -EIO;
  }
  else {
    printk(KERN_INFO "xilaxitimer_init: Registered IRQ %d\n", IR1);
  }
  irq_set_irq_type(IR1, IRQ_TYPE_EDGE_RISING);
  
  pdev0 = platform_device_register_simple(DEVICE_NAME0, 0, NULL, 0);              
  if (pdev0 == NULL) {                                                     
    printk(KERN_WARNING "xilaxitimer_init: Adding platform device \"%s\" failed\n", DEVICE_NAME0);
    kfree(pdev0);
    return -ENODEV;
  }                                                             
  pdev1 = platform_device_register_simple(DEVICE_NAME1, 0, NULL, 0);              
  if (pdev1 == NULL) {                                                     
    printk(KERN_WARNING "xilaxitimer_init: Adding platform device \"%s\" failed\n", DEVICE_NAME1);
    kfree(pdev1);                                                             
    return -ENODEV;                                                          
  }
  
  printk("dma_init: contains netmod_init\n");
  dma_init();


  return 0;
} 

static void __exit ack_exit(void)       
{
  /* 
   * Exit Device Module
   */
  //iounmap(dev_virtaddr);
  free_irq(IR0, NULL);
  free_irq(IR1, NULL);
  platform_device_unregister(pdev0);                                             
  platform_device_unregister(pdev1);                                                                                        
  printk(KERN_INFO "xilaxitimer_edit: Exit Device Module\n");

  //exit dma
  dma_exit();

}

module_init(ack_init);
module_exit(ack_exit);

MODULE_AUTHOR ("Xilinx");
MODULE_DESCRIPTION("Test Driver for Zynq PL AXI Timer.");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:xilaxitimer");
