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
#define DRV_RES_FAILURE -1
#define DRV_TARGET_PORT 32


static struct packet_type pack_type;
static struct net_device * netdev = NULL;
static struct net_device_stats netdev_statistics;

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
    uint16_t sport = htons(udp_header->source);
    uint16_t dport = htons(udp_header->dest);

    if (dport != DRV_TARGET_PORT)
        return;

    printk(KERN_INFO "UDP packet is received: {'sport': %d; 'dport': %d}\n",
           sport,
           dport);
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


//
// Stubs
//


static int ndev_open(struct net_device * dev)
{
    DRV_LOG_CTX_SET("open");
    LG_INF("Device opened");
    return DRV_RES_SUCCESS;
}


static int ndev_stop(struct net_device * dev)
{
    DRV_LOG_CTX_SET("stop");
    LG_INF("Device stopped");
    return DRV_RES_SUCCESS;
}


static int ndev_hard_start_xmit(struct sk_buff * skb, struct net_device * dev)
{
    DRV_LOG_CTX_SET("xmit");
    LG_INF("Start transmitting");
    return DRV_RES_SUCCESS;
}


static void ndev_tx_timeout(struct net_device * dev)
{
    DRV_LOG_CTX_SET("timeout");
    LG_INF("Timed out");
}


static struct net_device_stats * ndev_get_stats(struct net_device * dev)
{
    DRV_LOG_CTX_SET("device_stats")
    LG_INF("Retrieving device stats");
    return &netdev_statistics;
}


static int ndev_set_config(struct net_device * dev, struct ifmap * map)
{
    DRV_LOG_CTX_SET("config");
    LG_INF("Setting new config");
    return DRV_RES_SUCCESS;
}


static struct net_device_ops ndev_ops = {
    .ndo_open = ndev_open,
    .ndo_stop = ndev_stop,
    .ndo_start_xmit = ndev_hard_start_xmit,
    .ndo_tx_timeout = ndev_tx_timeout,
    .ndo_get_stats = ndev_get_stats,
    .ndo_set_config = ndev_set_config,
};


static int setup_network_interface(void)
{
    DRV_LOG_CTX_SET("setup_netwk")

    netdev = alloc_etherdev(20);
    if (netdev == NULL) {
        LG_FAILED_TO("allocate ethernet device");
        goto out;
    }

    netdev->netdev_ops = &ndev_ops;
    if (register_netdev(netdev)) {
        LG_FAILED_TO("register network device");
        goto release_netdev;
    }

    return DRV_RES_SUCCESS;

release_netdev:
    free_netdev(netdev);
out:
    return DRV_RES_FAILURE;
}


static void release_network_interface(void)
{
    unregister_netdev(netdev);
    free_netdev(netdev);
}


static int __init drv_init(void)
{
    DRV_LOG_CTX_SET("drv_init");
    LG_INF("Initializing the module");

    if (setup_packet_interception())
        return DRV_RES_FAILURE;
    if (setup_network_interface())
        return DRV_RES_FAILURE;
    return DRV_RES_SUCCESS;
}


static void __exit drv_exit(void)
{
    DRV_LOG_CTX_SET("drv_init");
    LG_INF("Cleaning up the module");

    release_network_interface();
    release_packet_interception();
}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
