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

#define INADDR_SEND  ((unsigned long int) 0xc0a80302U) //192.168.3.2 // for test

const unsigned char VLCRXMAC[ETH_ALEN] = {0xBC, 0x5F, 0xF4, 0x9A, 0x3B, 0x63};
extern int dma_init (void);
extern void dma_exit (void);
extern void dma_write_simple(char* buff, unsigned int buff_len);


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
    struct udphdr *udp_header;
    struct ethhdr *eth_header = (struct ethhdr *)skb_mac_header(skb);
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb); 
    // unsigned char seq;
    unsigned int i = 0;
    // char *buff;
    // skb: eth0 packet
    if (in == dev_eth){
        // return NF_ACCEPT;
        bufflen = skb->len;

        buff = (char*) kmalloc(MAX_SKB, GFP_KERNEL);
        // alias mac

        memcpy(buff, eth_header, ETH_HLEN);
        memcpy(buff, VLCRXMAC, ETH_ALEN); // overide dest MAC
        // alias ip
        ip_header->daddr    = htonl(INADDR_SEND); // overside dest IP address

        skb_copy_bits(skb, 0, buff + ETH_HLEN, bufflen);
        // write to dmacs
        printk("@@ received skb: ");
        for (i = 0; i < bufflen; i++){
            printk("%02x", buff[i]);
            if (i % 4 == 3){
                printk(" ");
            }
        }
        printk("\n");

        // buff, bufflen -> dma
        dma_write_simple(buff, bufflen + ETH_HLEN);

        kfree(buff);

        return NF_DROP;
    }

    // eth_header = (struct ethhdr*) skb_mac_header(skb);
    if (in == dev_wlan){
        // return NF_DROP;
        // printk("@@ Got some packet from wlan0\n");
        if (ip_header->protocol == IPPROTO_UDP) {
        	printk("@@ Got some UDP packet from wlan0\n");
            udp_header = (struct udphdr *)skb_transport_header(skb);
            if (!memcmp(skb->data, "@@ACK", 5)){
                // seq = (unsigned char)(skb->data[5]);
                printk("@@ get ACK %d\n", 1);
                return NF_DROP;
            }
            else if(!memcmp(skb->data, "@@NAK", 5)){
                // seq = (unsigned char)(skb->data[5]);
                printk("@@ get NAK %d\n", 1);
                return NF_DROP;
            }
            // selective repeat ARQ

        }
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops nfho_1 = {
    .hook       = (nf_hookfn*) hook_func_eth2dma,
    // .hooknum    = NF_BR_PRE_ROUTING, // just after come in
    .hooknum    = NF_INET_PRE_ROUTING, // just after come in
    // .pf         = NF_BRIDGE, // PF_INET for layer 3 alternate solution
    .pf         = PF_INET, // PF_INET for layer 3 alternate solution
    // .priority   = NF_BR_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
    .priority   = NF_IP_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
};

static int __init init_nf(void)
{
    printk("@@ size of eth_header: %d", sizeof(struct ethhdr));
    dev_eth = dev_get_by_name(&init_net, "eth0");
    dev_wlan = dev_get_by_name(&init_net, "wlan0");
    printk(KERN_INFO "@@ Register netfilter module.\n");
    nf_register_hook(&nfho_1);
    // nf_register_hook(&nfho_2);
    dma_init();




    return 0;
}

static void __exit exit_nf(void)
{
    printk(KERN_INFO "@@ Unregister netfilter module.\n");
    // nf_unregister_hook(&nfho_2);
    nf_unregister_hook(&nfho_1);

    dma_exit();

}

module_init(init_nf);
module_exit(exit_nf);
MODULE_LICENSE("GPL");
