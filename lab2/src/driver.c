#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "constants.h"
#include "logging.h"


struct drv_blkdev
{
    int minors;
    struct gendisk * gd;
    struct request_queue * queue;
    spinlock_t lock;
};

static struct
{
    int blk_major;
    struct drv_blkdev blkdev;
    struct block_device_operations blk_ops;
} module_globals = {.blk_major = 0, .blk_ops = {.owner = THIS_MODULE}};


static void drv_request_handler(struct request_queue * queue) {}


static int drv_gendisk_create(struct drv_blkdev * blkdev)
{
    DRV_LOG_CTX_SET("drv_gendisk_create");

    LG_DBG("Alloc gendisk");
    blkdev->gd = alloc_disk(blkdev->minors);
    if (blkdev->gd)
        return -ENOMEM;

    LG_DBG("Initialize gendisk");
    blkdev->gd->major = module_globals.blk_major;
    blkdev->gd->first_minor = 0;
    blkdev->gd->fops = &module_globals.blk_ops;
    blkdev->gd->queue = blkdev->queue;
    blkdev->gd->private_data = blkdev;

    snprintf(blkdev->gd->disk_name, DRV_DISKNAME_MAX, DRV_NAME);
    set_capacity(blkdev->gd, DRV_NSECTORS);

    LG_DBG("Adding gendisk into the system");
    add_disk(blkdev->gd);

    return DRV_OP_SUCCESS;
}


static void drv_gendisk_delete(struct gendisk * gd)
{
    if (gd)
        del_gendisk(gd);
}


static int drv_blkdev_init(struct drv_blkdev * blkdev, int minors)
{
    DRV_LOG_CTX_SET("drv_blkdev_init");

    blkdev->minors = minors;

    LG_DBG("Initialize queue");
    spin_lock_init(&blkdev->lock);
    blkdev->queue = blk_init_queue(drv_request_handler, &blkdev->lock);
    if (!blkdev->queue) {
        LG_FAILED_TO("initialize requests queue");
        goto out;
    }

    LG_DBG("Setting blk logical size");
    blk_queue_logical_block_size(blkdev->queue, DRV_SECTOR_SZ);
    blkdev->queue->queuedata = blkdev;

    LG_DBG("Create gendisk");
    if (drv_gendisk_create(blkdev) < 0) {
        LG_FAILED_TO("create gendisk");
        goto undo_blk_queue_init;
    }

    return DRV_OP_SUCCESS;

undo_blk_queue_init:
    blk_cleanup_queue(blkdev->queue);
out:
    return -ENOMEM;
}


static void drv_blkdev_deinit(struct drv_blkdev * blkdev)
{
    drv_gendisk_delete(blkdev->gd);
    blk_cleanup_queue(blkdev->queue);
}


static int __init drv_init(void)
{
    int status = 0;
    DRV_LOG_CTX_SET("drv_init");
    LG_INF("Start module initialization");

    LG_DBG("Register blkdev");
    module_globals.blk_major
        = register_blkdev(module_globals.blk_major, DRV_NAME);
    if (module_globals.blk_major <= 0) {
        LG_FAILED_TO("register blkdev");
        status = module_globals.blk_major;
        goto out;
    }

    LG_DBG("Initialize blkdev");
    status = drv_blkdev_init(&module_globals.blkdev, DRV_MINORS);
    if (status < 0) {
        LG_FAILED_TO("initialize blkdev");
        goto undo_blkdev_reg;
    }

    return DRV_OP_SUCCESS;

undo_blkdev_reg:
    unregister_blkdev(module_globals.blk_major, DRV_NAME);
out:
    return status;
}


static void __exit drv_exit(void)
{
    drv_blkdev_deinit(&module_globals.blkdev);
    unregister_blkdev(module_globals.blk_major, DRV_NAME);
}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
