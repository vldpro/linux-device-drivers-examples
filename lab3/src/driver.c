#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/version.h>

#define DRV_LOG_DISABLE_DEBUG

#include "logging.h"


#define DRV_RES_SUCCESS 0


static struct packet_type pack_type;


//
// Helpers
//


static inline struct iphdr * get_ip_header(struct sk_buff * skb)
{
    return (struct iphdr *)skb_network_header(skb);
}


static inline struct udphdr * get_udp_header(struct sk_buff * skb)
{
    return (struct udphdr *)skb_transport_header(skb);
}


static inline int network_layer_is_ip(struct sk_buff * skb)
{
    return skb->protocol == htons(ETH_P_IP);
}


static inline int transport_layer_is_udp(struct sk_buff * skb)
{
    struct iphdr * ip_header = get_ip_header(skb);
    return ip_header->protocol == IPPROTO_UDP;
}


//
// Main logic
//


void process_skbuff_with_udp_packet(struct sk_buff * skb)
{
    struct udphdr * udp_header = get_udp_header(skb);
    uint16_t sport = udp_header->source;
    uint16_t dport = udp_header->dest;

    printk(KERN_INFO "UDP packet is received: {'sport': %d; 'dport': %d}\n",
           htons(sport),
           htons(dport));
}


int handle_packet(struct sk_buff * skb,
                  struct net_device * dev,
                  struct packet_type * ptype,
                  struct net_device * orig_dev)
{
    DRV_LOG_CTX_SET("packet_handler");

    if (!network_layer_is_ip(skb)) {
        LG_DBG("Invalid network layer protocol. IP is expected. Skipping");
        return DRV_RES_SUCCESS;
    }

    if (!transport_layer_is_udp(skb)) {
        LG_DBG("Invalid transport layer protocol. UDP is expected. Skipping");
        return DRV_RES_SUCCESS;
    }

    process_skbuff_with_udp_packet(skb);
    return DRV_RES_SUCCESS;
}


static int setup_packet_interception(void)
{
    pack_type.type = htons(ETH_P_IP);
    pack_type.dev = NULL;
    pack_type.func = handle_packet;

    dev_add_pack(&pack_type);
    return DRV_RES_SUCCESS;
}


static void release_packet_interception(void)
{
    dev_remove_pack(&pack_type);
}


static int __init drv_init(void)
{
    DRV_LOG_CTX_SET("drv_init");
    LG_INF("Initializing the module");

    setup_packet_interception();
    return DRV_RES_SUCCESS;
}


static void __exit drv_exit(void)
{
    DRV_LOG_CTX_SET("drv_init");
    LG_INF("Cleaning up the module");

    release_packet_interception();
}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
