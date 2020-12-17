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

static struct net_device* dev_eth;
static struct net_device* dev_wlan;

/* This function to be called by hook. */
static unsigned int
hook_func_prerouting(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn) (struct sk_buff *))
{
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb); 
    // unsigned char seq;
    // char *buff;
    // skb: eth0 packet
    if (in == dev_wlan){

        printk("## PREROUTING: Received eth packet: \n");
        printk("## source_ip: %x, dest_ip: %x\n", ntohl(ip_header->saddr), ntohl(ip_header->daddr));

        return NF_ACCEPT;
    }

    return NF_ACCEPT;

}

static unsigned int
hook_func_postrouting(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn) (struct sk_buff *))
{
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb); 
    // unsigned char seq;
    // char *buff;
    // skb: eth0 packet
    if (out == dev_eth){

        printk("## POSTROUTING: Received eth packet: \n");
        printk("## source_ip: %x, dest_ip: %x\n", ntohl(ip_header->saddr), ntohl(ip_header->daddr));

        return NF_ACCEPT;
    }

    return NF_ACCEPT;

}

static struct nf_hook_ops nfho_1 = {
    .hook       = (nf_hookfn*) hook_func_prerouting,
    // .hooknum    = NF_BR_PRE_ROUTING, // just after come in
    .hooknum    = NF_INET_PRE_ROUTING, // just after come in
    // .pf         = NF_BRIDGE, // PF_INET for layer 3 alternate solution
    .pf         = PF_INET, // PF_INET for layer 3 alternate solution
    // .priority   = NF_BR_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
    .priority   = NF_IP_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
};

static struct nf_hook_ops nfho_2 = {
    .hook       = (nf_hookfn*) hook_func_postrouting,
    // .hooknum    = NF_BR_PRE_ROUTING, // just after come in
    .hooknum    = NF_INET_POST_ROUTING, // just after come in
    // .pf         = NF_BRIDGE, // PF_INET for layer 3 alternate solution
    .pf         = PF_INET, // PF_INET for layer 3 alternate solution
    // .priority   = NF_BR_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
    .priority   = NF_IP_PRI_LAST,// NF_IP_PRI_FIRST for layer 3 alternate solution
};

static int __init init_nf(void)
{
    printk("@@ size of eth_header: %d", sizeof(struct ethhdr));
    dev_eth = dev_get_by_name(&init_net, "eth0");
    dev_wlan = dev_get_by_name(&init_net, "wlan1");
    printk(KERN_INFO "@@ Register netfilter module.\n");
    nf_register_hook(&nfho_1);
    nf_register_hook(&nfho_2);
    // dma_init();




    return 0;
}

static void __exit exit_nf(void)
{
    printk(KERN_INFO "@@ Unregister netfilter module.\n");
    // nf_unregister_hook(&nfho_2);
    nf_unregister_hook(&nfho_1);

    // dma_exit();

}

module_init(init_nf);
module_exit(exit_nf);
MODULE_LICENSE("GPL");
