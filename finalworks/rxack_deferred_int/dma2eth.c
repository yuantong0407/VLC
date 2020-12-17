#include <linux/init.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/netpoll.h>
#include <linux/udp.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <asm/unaligned.h>
#include <net/addrconf.h>
#include <net/net_namespace.h>
#include <linux/interrupt.h>

#define MAX_UDP_CHUNK 1460

#define MESSAGE_SIZE 1024
#define INADDR_LOCAL ((unsigned long int) 0xc0a80165U) //192.168.1.101
#define INADDR_SEND  ((unsigned long int) 0xc0a8017bU) //192.168.1.123
// #define INADDR_SEND  ((unsigned long int) 0xc0a80175U) //192.168.1.117

const unsigned char MY_MAC[ETH_ALEN] = {0x00, 0x0A, 0x35, 0xDE, 0xAD, 0xBE}; // rx eth
// const unsigned char HER_MAC[ETH_ALEN] = {0xE8, 0xB1, 0xFC, 0x33, 0x3C, 0x36}; // ThinkPad wlan
// const unsigned char HER_MAC[ETH_ALEN] = {0xBC, 0x5F, 0xF4, 0x9A, 0x3B, 0x63}; // BigComputer Eth
// const unsigned char HER_MAC[ETH_ALEN] = {0x68, 0xF7, 0x28, 0x08, 0x6a, 0x96}; // ThinkPad eth
// const unsigned char HER_MAC[ETH_ALEN] = {0xE8, 0x03, 0x9A, 0xDB, 0x66, 0xC4}; // Samsung Q470 eth
// const unsigned char HER_MAC[ETH_ALEN] = {0xC8, 0x60, 0x00, 0xC5, 0x05, 0xEA};   //windows
// const unsigned char HER_MAC[ETH_ALEN] = {0x3C, 0x46, 0xD8, 0x93, 0x77, 0x3B}; // 
// const unsigned char HER_MAC[ETH_ALEN] = {0x2C, 0x56, 0xDC, 0x94, 0x74, 0xCB}; // feiye's mac
const unsigned char HER_MAC[ETH_ALEN] = {0xA0, 0xB3, 0xCC, 0x23, 0xAF, 0x94}; // feiye's mac

// static char npname[16] = "deadbeefbaadbeef";
// static struct netpoll np0 = {0, "eth0", npname};
// static struct netpoll *np = &np0;
static struct sk_buff *skb;
static struct net_device* dev;

int sendskb(unsigned char* dmadata, int dmadata_len);
int netmod_init(void);

int sendskb(unsigned char* dmadata, int dmadata_len){
	struct ethhdr *eth;
	int ret;
	
	// printk("@@netpoll_setup\n");

	skb = alloc_skb(dmadata_len + dev->needed_tailroom, GFP_ATOMIC);
	if (!skb){
		return 1;
	}
	atomic_set(&skb->users, 1);
	// skb_reserve(skb, total_len - len);

	// printk("@@skb_reserve\n");	
	
	skb_copy_to_linear_data(skb, dmadata, dmadata_len);
    skb_put(skb, dmadata_len);

    skb_reset_mac_header(skb);
	eth = (struct ethhdr*) skb_mac_header(skb);
	
	memcpy(eth->h_source, dev->dev_addr, ETH_ALEN);
	memcpy(eth->h_dest, HER_MAC, ETH_ALEN);
	
	skb->protocol = htons(ETH_P_IP);
	
	skb->dev = dev;

	ret = dev_queue_xmit(skb);

	printk("@@ Send to ethernet, returned %d.\n", ret);	

	return ret; // you gonna fly high, you'll never gonna die, you'll gonna make it if you try
}


int netmod_init(void){

    dev = dev_get_by_name(&init_net, "eth0");

    return 0; // you gonna fly high youll never gonna die

}