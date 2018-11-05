#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/spinlock.h>

#include "constants.h"
#include "logging.h"


struct drv_dev {
    int sz;
    uint8_t* vdisk;
    spinlock_t spinlock;
    struct request_queue *queue;
    struct gendisk *gendisk;
};


static int __init drv_init(void) 
{
    DRV_LOG_INIT(INFO, "Starting initialization\n");
    int major = register_blkdev(15, DRV_NAME);

    if (major <= 0) {
        DRV_LOG_INIT(ERR, "Could not register block device\n");
        goto err;
    }

undo_blkdev_reg:
    unregister_blkdev(major, DRV_NAME);
err:
    return -EBUSY; 
}


static void __exit drv_exit(void) { }


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
