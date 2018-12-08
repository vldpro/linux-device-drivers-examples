#include <linux/etherdevice.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/version.h>


#define DRV_INTERFACE_NAME "ndev%d"
#define DRV_RES_SUCCESS 0

struct dev_priv
{
    struct net_device_stats stats;
    int status;
    struct sk_buff * skb;
    spinlock_t lock;
};

static struct
{
    struct net_device * dev;
} mod = {.dev = NULL};


static int ndev_open(struct net_device * dev)
{
    memcpy(dev->dev_addr, "\0ndev0", ETH_ALEN);
    netif_start_queue(dev);
    return DRV_RES_SUCCESS;
}


static int ndev_release(struct ned_device * dev)
{
    netif_stop_queue(dev);
}


static void ndev_init(struct net_device * dev)
{
    ether_setup(dev);
    struct dev_priv * priv = netdev_priv(dev);

    memset(priv, 0, sizeof(struct dev_priv));
    spin_lock_init(&priv->lock)
}


static int __init drv_init(void)
{
    DRV_LOG_CTX_SET("drv_init");
    LG_DBG("Initializing network interface...");

    mod.dev = alloc_netdev(0, DRV_INTERFACE_NAME, ndev_init);
    if (mod.dev == NULL) {
        LG_FAILED_TO("alloc network device.");
        return ENOMEM;
    }

    int res = register_netdev(mod.dev);
    if (res != DRV_RES_SUCCESS) {
        LG_FAILED_TO("register network device.");
        return res;
    }

    return DRV_RES_SUCCESS;
}


static void __exit drv_exit(void)
{
    DRV_LOG_CTX_SET("drv_exit");
    LG_INF("Deinitializing module");
    unregister_netdev(mod.dev);
    free_netdev(mod.dev);
}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
