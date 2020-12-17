#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/net_namespace.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/ip.h>

#define MAX_SKB 1536
#define TX_WINSIZE 256
#define TX_WINBYTE 1
// #define ETH_HLEN sizeof(struct ethhdr)

#define ARQ_STOP_N_WAIT 

#define INADDR_SEND  ((unsigned long int) 0xc0a80302U) //192.168.3.2 // for test
#define IP_RXADDR  ((unsigned long int) 0xc0a80202U) //192.168.2.2 // for test
#define IP_TXADDR  ((unsigned long int) 0xc0a80201U) //192.168.2.1 // for test

const unsigned char VLCRXMAC[ETH_ALEN] = {0xBC, 0x5F, 0xF4, 0x9A, 0x3B, 0x63};
extern int dma_init (void);
extern void dma_exit (void);
extern void dma_write_simple(char* buff, unsigned int buff_len);

// stop and wait
// extern void snw_init(void);
// extern int snw_send(char* buff, unsigned int bufflen);
// extern int snw_handle_ack(void);
// extern int snw_handle_nak(void);

static struct net_device* dev_eth;
static struct net_device* dev_wlan;

/* This function to be called by hook. */
static unsigned int
hook_func_eth2dma(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn) (struct sk_buff *))
{
    char *buff;
    unsigned int bufflen;
    struct ethhdr *eth_header = (struct ethhdr *)skb_mac_header(skb);
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb); 
    unsigned int i = 0;
    // char *buff;
    // skb: eth0 packet
    if (out == dev_wlan && ip_header->daddr == htonl(IP_RXADDR)){
        // return NF_ACCEPT;
        bufflen = skb->len;

        buff = (char*) kmalloc(MAX_SKB, GFP_KERNEL);
        if (!buff){
        	printk("@@ kernel no space for packet %d\n", ip_header->id);
        	return NF_DROP;
        }
        // alias mac

        memcpy(buff, eth_header, ETH_HLEN);
        memcpy(buff, VLCRXMAC, ETH_ALEN); // overide dest MAC
        // alias ip
        ip_header->daddr    = htonl(INADDR_SEND); // overside dest IP address

        skb_copy_bits(skb, 0, buff + ETH_HLEN, bufflen);
        // write to dmacs
        // printk("@@ Ready to transfer to DMA: ");
        // for (i = 0; i < bufflen; i++){
        //     printk("%02x", buff[i]);
        //     if (i % 4 == 3){
        //         printk(" ");
        //     }
        // }
        // printk("\n");

        // buff, bufflen -> dma
        // snw_send(buff, bufflen + ETH_HLEN);
        dma_write_simple(buff, bufflen + ETH_HLEN);

        // kfree(buff);

        return NF_DROP;
    }

    return NF_ACCEPT;
}

static unsigned int
hook_func_ack(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn) (struct sk_buff *))
{
    // struct ethhdr *eth_header = (struct ethhdr *)skb_mac_header(skb);
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb); 
    struct udphdr *udp_header;
    // unsigned char seq;
    int ret;
    // unsigned char buff[6];
    unsigned char* p;

    // eth_header = (struct ethhdr*) skb_mac_header(skb);
    if (in == dev_wlan && ip_header->daddr == htonl(IP_TXADDR)){
        // return NF_DROP;
        // printk("@@ Got some packet from wlan0\n");
        if (ip_header->protocol == IPPROTO_UDP) {
            // printk("@@ Got some UDP packet from wlan0\n");
            udp_header = (struct udphdr *)skb_transport_header(skb);
            // skb_copy_bits(skb, (ip_header->ihl << 2) + udp_header->len, buff, 6);
            // printk("@@ Content: ");
            // printk("%02x ", buff[0]);
            // printk("%02x ", buff[1]);
            // printk("%02x ", buff[2]);
            // printk("%02x ", buff[3]);
            // printk("%02x \n", buff[4]);
            ret = skb_linearize(skb);
            printk("@@ linearize returned %d\n", ret);
            p = skb->data + (ip_header -> ihl << 2) + sizeof(struct udphdr);
            // printk("@@ ip header length: %d, udp header length: %d.\n", (ip_header -> ihl << 2), sizeof(struct udphdr));
            // printk("@@ All Content: ");
   //          for (i = 0; i < 20; i++){
	  //           printk("%02x", skb->data[i]);
   //  	    }
   //          printk("\n");
			// // 45000022000100004011f576c0a80202c0a80201
         //    printk("@@ Content: ");
         //    for (i = 0; i < 6; i++){
	        //     printk("%02x", p[i]);
    	    // }
    	    // ip header
    	    // 4500 0022 
    	    // 0001 0000
    	    // 4011 f576 
    	    // c0a8 0202
    	    // c0a8 0201
    	    // udp header 
    	    // 1a09 1a0a
    	    // 000e 7986 
    	    // content
    	    // 4040 4143
    	    // 4b61
            // printk("\n");
            if (!memcmp(p, "@@ACK", 5)){
                // seq = (unsigned char)(p[5]);
                printk("@@ get ACK %d\n", p[5]);
                // handle ack
                // snw_handle_ack();
                return NF_DROP;
            }
            else if(!memcmp(p, "@@NAK", 5)){
                // seq = (unsigned char)(p[5]);
                printk("@@ get NAK %d\n", p[5]);

                // handle nak
                // snw_handle_nak();
                return NF_DROP;
            }
            // selective repeat ARQ

            return NF_DROP;
        }
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops nfho_1 = {
    .hook       = (nf_hookfn*) hook_func_eth2dma,
    // .hooknum    = NF_BR_PRE_ROUTING, // just after come in
    .hooknum    = NF_INET_POST_ROUTING, // to out
    // .pf         = NF_BRIDGE, // PF_INET for layer 3 alternate solution
    .pf         = PF_INET, // PF_INET for layer 3 alternate solution
    // .priority   = NF_BR_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
    .priority   = NF_IP_PRI_CONNTRACK_DEFRAG - 1,// NF_IP_PRI_FIRST for layer 3 alternate solution
};

static struct nf_hook_ops nfho_2 = {
    .hook       = (nf_hookfn*) hook_func_ack,
    // .hooknum    = NF_BR_PRE_ROUTING, // just after come in
    .hooknum    = NF_INET_PRE_ROUTING, // to local
    // .pf         = NF_BRIDGE, // PF_INET for layer 3 alternate solution
    .pf         = PF_INET, // PF_INET for layer 3 alternate solution
    // .priority   = NF_BR_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
    .priority   = NF_IP_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
};


/*

A Packet Traversing the Netfilter System:

   --->[0]--->[ROUTE]--->[2]--->[4]--->
                 |            ^
                 |            |
                 |         [ROUTE]
                 v            |
                [1]          [3]
                 |            ^
                 |            |
                 v            |

 After promisc drops, checksum checks.
#define NF_IP_PRE_ROUTING   0

 If the packet is destined for this box. 
#define NF_IP_LOCAL_IN      1

 If the packet is destined for another interface. 
#define NF_IP_FORWARD       2

 Packets coming from a local process. 
#define NF_IP_LOCAL_OUT     3

 Packets about to hit the wire. 
#define NF_IP_POST_ROUTING  4

   --->PRE------>[ROUTE]--->FWD---------->POST------>
       Conntrack    |       Mangle   ^    Mangle
       Mangle       |       Filter   |    NAT (Src)
       NAT (Dst)    |                |    Conntrack
       (QDisc)      |             [ROUTE]
                    v                |
                    IN Filter       OUT Conntrack
                    |  Conntrack     ^  Mangle
                    |  Mangle        |  NAT (Dst)
                    v                |  Filter

enum nf_ip_hook_priorities {
    NF_IP_PRI_FIRST = INT_MIN,
    NF_IP_PRI_CONNTRACK_DEFRAG = -400,
    NF_IP_PRI_RAW = -300,
    NF_IP_PRI_SELINUX_FIRST = -225,
    NF_IP_PRI_CONNTRACK = -200,
    NF_IP_PRI_MANGLE = -150,
    NF_IP_PRI_NAT_DST = -100,
    NF_IP_PRI_FILTER = 0,
    NF_IP_PRI_SECURITY = 50,
    NF_IP_PRI_NAT_SRC = 100,
    NF_IP_PRI_SELINUX_LAST = 225,
    NF_IP_PRI_CONNTRACK_HELPER = 300,
    NF_IP_PRI_CONNTRACK_CONFIRM = INT_MAX,
    NF_IP_PRI_LAST = INT_MAX,
};

*/

static int __init init_nf(void)
{
    // printk("@@ size of eth_header: %d", sizeof(struct ethhdr));
    dev_eth = dev_get_by_name(&init_net, "eth0");
    dev_wlan = dev_get_by_name(&init_net, "wlan1");
    printk(KERN_INFO "@@ Register netfilter module.\n");
    nf_register_hook(&nfho_1);
    nf_register_hook(&nfho_2);
    dma_init();

    printk(KERN_INFO "@@ Version v19: comment on comment\n");
    // snw_init();


    return 0;
}

static void __exit exit_nf(void)
{
    printk(KERN_INFO "@@ Unregister netfilter module.\n");
    nf_unregister_hook(&nfho_2);
    nf_unregister_hook(&nfho_1);

    dma_exit();

}

module_init(init_nf);
module_exit(exit_nf);
MODULE_LICENSE("GPL");
