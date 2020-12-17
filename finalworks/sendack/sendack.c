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

static struct net_device* dev;
static struct sk_buff *skb;

#define INADDR_FROM ((unsigned long int) 0xc0a80202U) //192.168.2.2
#define INADDR_TO  ((unsigned long int) 0xc0a80201U) //192.168.2.1
// #define INADDR_FROM  ((unsigned long int) 0xc0a80204U) //192.168.2.3 // for test

const unsigned char TO_MAC[ETH_ALEN] = {0x3C, 0x46, 0xD8, 0x93, 0x77, 0x3B}; // tx
const unsigned char FROM_MAC[ETH_ALEN] = {0xEC, 0x08, 0x6B, 0x1F, 0x96, 0x99}; // rx ec:08:6b:1f:96:99
// const unsigned char TO_MAC[ETH_ALEN] = {0xE8, 0x03, 0x9A, 0xFB, 0x01, 0x72}; // Samsung WiFi


static int sendsomething(const char* message, unsigned int len){
  int total_len, eth_len, ip_len, udp_len;
  int ret;
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
  udph->check = 0;
  udph->check = csum_tcpudp_magic(htonl(INADDR_FROM),
    htonl(INADDR_TO),
    udp_len, IPPROTO_UDP,
    csum_partial(udph, udp_len, 0));

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
  iph->saddr    = htonl(INADDR_FROM);
  iph->daddr    = htonl(INADDR_TO);
  iph->check    = ip_fast_csum((unsigned char *)iph, iph->ihl);

  eth = (struct ethhdr *) skb_push(skb, ETH_HLEN);
  skb_reset_mac_header(skb);

  // printk("@@skb_push skb\n"); 
  
  eth->h_proto = htons(ETH_P_IP);
  memcpy(eth->h_source, FROM_MAC, ETH_ALEN);
  memcpy(eth->h_dest, TO_MAC, ETH_ALEN);
  
  skb->protocol = eth->h_proto;
  // printk("@@%lu %lu\n", sizeof(*eth->h_source), sizeof(*eth->h_dest));
  
  skb->dev = dev;

  ret = dev_queue_xmit(skb);

  // printk("@@ Send wifi packet, returned %d.\n", ret); 

  return ret; // you gonna fly high, you'll never gonna die, you'll gonna make it if you try
}

static int __init init_sendack(void){
  int ret;
  dev = dev_get_by_name(&init_net, "wlan0");

  ret = sendsomething("@@ACKa", 6);
  printk("@@ Module version v1.0\n");
  printk("@@ send with return %d\n", ret);
  return 0;
}

static void __exit ack_exit(void)       
{

}

module_init(init_sendack);
module_exit(ack_exit);

MODULE_AUTHOR ("Dengwang Tang");
MODULE_DESCRIPTION("must send an ack");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:xilaxitimer");
