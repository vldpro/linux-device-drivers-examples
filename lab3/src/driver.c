#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/version.h>


#define DRV_INTERFACE_NAME "ndev%d"
#define DRV_RES_SUCCESS 0


static struct
{
    struct net_device * dev;
} mod = {.dev = NULL};


static int __init drv_init(void)
{
    DRV_LOG_CTX_SET("drv_init");
    LG_DBG("Initializing network interface...");

    mod.dev = alloc_netdev(0, DRV_INTERFACE_NAME, NULL);
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


static void __exit drv_exit(void) {}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
