#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <net/net_namespace.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/ip.h>

#define MAX_SKB 1536
#define TX_WINSIZE 256
#define TX_WINBYTE 1
// #define ETH_HLEN sizeof(struct ethhdr)

#define INADDR_RX ((unsigned long int) 0xc0a80202U) //192.168.2.2
// #define INADDR_TX  ((unsigned long int) 0xc0a80201U) //192.168.2.1
// #define INADDR_TX  ((unsigned long int) 0xc0a80203U) //192.168.2.3 // for test

extern void ip_send_check(struct iphdr *iph);
// extern int skb_make_writable(struct sk_buff *skb, unsigned int writable_len);

static struct net_device* dev_eth;
static struct net_device* dev_wlan;

static uint16_t csum(uint16_t* buff, int nwords) {
    uint32_t sum;
    for (sum = 0; nwords > 0; nwords--)
            sum += *buff++;
    sum = (sum >> 16) + (sum & 0xffff);
    // sum += (sum >> 16);
    sum = (sum >> 16) + (sum & 0xffff);

    return ((uint16_t) ~sum);
}

static unsigned int
hook_func_postrouting(unsigned int hooknum,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn) (struct sk_buff *))
{
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb); 
    struct udphdr *udp_header;
    struct tcphdr *tcp_header;
    unsigned int udp_packet_len;
    unsigned int tcp_packet_len;
    int ret;

    skb_set_transport_header(skb, (ip_header->ihl << 2));
    udp_header = (struct udphdr *)skb_transport_header(skb);
    tcp_header = (struct tcphdr *)skb_transport_header(skb);
    udp_packet_len = skb->len - (ip_header->ihl << 2);
    tcp_packet_len = skb->len - (ip_header->ihl << 2);

    // unsigned char seq;
    // char *buff;
    // skb: eth0 packet
    if (out == dev_wlan && ip_header->saddr != htonl(INADDR_RX)){
        // printk("@@ ip_header pointer: %lx", (unsigned long int)ip_header);
        // printk("@@ udp_header pointer: %lx", (unsigned long int)udp_header);

        // printk("## POSTROUTING: Received eth packet: \n");
        // printk("## source_ip: %x, dest_ip: %x\n", ntohl(ip_header->saddr), ntohl(ip_header->daddr));


        // linearize

        // printk("## skb_is %s linear\n", 
            // (skb_is_nonlinear(skb)?"not": ""));
        ret = skb_linearize(skb);
        // printk("## skb_linearize, returned %d\n", ret);

        ret = skb_make_writable(skb, skb->len);
        // printk("## skb_make_writeable, returned %d\n", ret);

        // printk("## id: %d, ORIGINAL cksum: %04x, OLD cksum: %04x\n", 
        //     ntohs(ip_header->id),
        //     ip_header->check,
        //     csum((uint16_t*) ip_header, (ip_header->ihl << 1)));

        ip_header->check = 0;
        // printk("## id: %d, MY COMPUTED OLD cksum: %04x", ntohs(ip_header->id),
        //     csum((uint16_t*) ip_header, (ip_header->ihl << 1)));
        
        // printk("## ttl: %d", ip_header->ttl);
        // printk("## flag & fragment offset: %04x\n", ntohs(ip_header->frag_off));

        // SNAT
        ip_header->saddr    = htonl(INADDR_RX);
        
        // checksum
        // printk("## id: %d, source_ip: %x, dest_ip: %x\n", 
            // ntohs(ip_header->id),
            // ntohl(ip_header->saddr), ntohl(ip_header->daddr));

        ip_header->check = 0;
        ip_header->check = csum((uint16_t*) ip_header, (ip_header->ihl << 1));

        // printk("## id: %d, NEW cksum: %04x\n",
        //     ntohs(ip_header->id), ip_header->check);

        // printk("## flag & fragment offset 0: %04x\n", ntohs(ip_header->frag_off)); 
        // TCP/UDP checksum
        if (ip_header->protocol == IPPROTO_TCP){
            // printk("## OUTPUT TCP packet\n");
            tcp_header->check = 0;
            tcp_header->check = csum_tcpudp_magic(
                ip_header->saddr,
                ip_header->daddr,
                tcp_packet_len, IPPROTO_TCP,
                csum_partial(tcp_header, tcp_packet_len, 0));
        }
        else if (ip_header->protocol == IPPROTO_UDP){
            // printk("## OUTPUT UDP packet\n");
            // printk("## flag & fragment offset 1: %04x\n", ntohs(ip_header->frag_off)); 
            udp_header->check = 0;
            udp_header->check = csum_tcpudp_magic(
                ip_header->saddr,
                ip_header->daddr,
                udp_packet_len, IPPROTO_UDP,
                csum_partial(udp_header, udp_packet_len, 0));
            // printk("## flag & fragment offset 2: %04x\n", ntohs(ip_header->frag_off)); 
        }

        // free CONNTRACK info
        nf_conntrack_put(skb->nfct);
        skb->nfct = NULL;

        // printk("## flag & fragment offset after: %04x\n", ntohs(ip_header->frag_off)); 

        return NF_ACCEPT;
    }

    return NF_ACCEPT;

}

static struct nf_hook_ops nfho_2 = {
    .hook       = (nf_hookfn*) hook_func_postrouting,
    // .hooknum    = NF_BR_PRE_ROUTING, // just after come in
    .hooknum    = NF_INET_POST_ROUTING, // just after come in
    // .pf         = NF_BRIDGE, // PF_INET for layer 3 alternate solution
    .pf         = PF_INET, // PF_INET for layer 3 alternate solution
    // .priority   = NF_BR_PRI_FIRST,// NF_IP_PRI_FIRST for layer 3 alternate solution
    .priority   = NF_IP_PRI_RAW,// NF_IP_PRI_FIRST for layer 3 alternate solution
};

static int __init init_nf(void)
{
    printk("@@ size of eth_header: %d", sizeof(struct ethhdr));
    dev_eth = dev_get_by_name(&init_net, "eth0");
    dev_wlan = dev_get_by_name(&init_net, "wlan0");
    printk(KERN_INFO "@@ Register netfilter module.\n");
    // nf_register_hook(&nfho_1);
    nf_register_hook(&nfho_2);
    printk(KERN_INFO "@@ rxup version v2.3: done!.\n");
    // dma_init();




    return 0;
}

static void __exit exit_nf(void)
{
    printk(KERN_INFO "@@ Unregister netfilter module.\n");
    nf_unregister_hook(&nfho_2);
    // nf_unregister_hook(&nfho_1);

    // dma_exit();

}

module_init(init_nf);
module_exit(exit_nf);
MODULE_LICENSE("GPL");
