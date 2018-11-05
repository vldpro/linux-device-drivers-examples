#include <linux/blkdev.h>
#include <linux/fs.h>
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
    int size;
    uint8_t * vdisk;
    spinlock_t lock;
    struct request_queue * queue;
    struct gendisk * gd;
};


struct drv_blkdev_geo
{
    int nsectors;
    int sector_sz;
};


static void drv_request(struct request_queue * queue) {}


static int drv_open(struct block_device * dev, fmode_t mode)
{
    return 0;
}


static void drv_release(struct gendisk * gd, fmode_t mode) {}


static struct block_device_operations blk_ops
    = {.open = drv_open, .release = drv_release, .owner = THIS_MODULE};


static struct gendisk * gendisk_create(struct drv_blkdev * bdev,
                                       struct block_device_operations * drv_ops,
                                       int major,
                                       int minors)
{
    int which = 0;
    struct gendisk * gd = alloc_disk(minors);

    if (!gd) {
        DRV_LOG_INIT(ERR, "Failed to alloc memory for gendisk");
        return NULL;
    }

    gd->major = major;
    gd->first_minor = which * minors;
    gd->fops = drv_ops;
    gd->queue = bdev->queue;
    gd->private_data = bdev;

    snprintf(gd->disk_name, 32, DRV_NAME "%c", which + 'a');
    set_capacity(gd, bdev->size);

    return gd;
}


static struct drv_blkdev *
bdev_create(int major, int minors, struct drv_blkdev_geo geo)
{
    // Alloc mem for block dev
    struct drv_blkdev * bdev = kmalloc(sizeof(struct block_device), GFP_KERNEL);
    if (!bdev) {
        DRV_LOG_INIT(ERR, "Failed to alloc memory for block_device");
        goto out;
    }

    memset(bdev, 0, sizeof(struct block_device));
    bdev->size = geo.nsectors * geo.sector_sz;
    bdev->vdisk = vmalloc(bdev->size);

    // Alloc mem for virtual disk
    if (bdev->vdisk == NULL) {
        DRV_LOG_INIT(ERR, "Failed to alloc memory for virtual disk");
        goto undo_bdev_alloc;
    }

    // Create block device request queue
    spin_lock_init(&bdev->lock);
    bdev->queue = blk_init_queue(drv_request, &bdev->lock);
    if (!bdev->queue) {
        DRV_LOG_INIT(ERR, "Failed to allocate memory for blk queue");
        goto undo_bdisk_alloc;
    }

    // Create gendisk structure
    bdev->gd = gendisk_create(bdev, &blk_ops, major, minors);
    if (!bdev->gd) {
        DRV_LOG_INIT(ERR, "Failed to allocate gendisk");
        goto undo_init_queue;
    }

    // Add disk to the system
    DRV_LOG_INIT(INFO, "Block device successfully created");
    return bdev;

undo_init_queue:
    vfree(bdev->queue);
undo_bdisk_alloc:
    vfree(bdev->vdisk);
undo_bdev_alloc:
    kfree(bdev);
out:
    return NULL;
}


/*static void bdev_delete(struct drv_blkdev * bdev)
{
    vfree(bdev->queue);
    vfree(bdev->vdisk);
    kfree(bdev);
}*/


static int __init drv_init(void)
{
    int major = 15;
    struct drv_blkdev * bdev = NULL;
    struct drv_blkdev_geo geo
        = {.nsectors = DRV_NSECTORS, .sector_sz = DRV_SECTOR_SZ};

    DRV_LOG_INIT(INFO, "Starting initialization\n");
    major = register_blkdev(major, DRV_NAME);

    if (major <= 0) {
        DRV_LOG_INIT(ERR, "Could not register block device\n");
        goto err;
    }

    bdev = bdev_create(major, DRV_MINORS, geo);
    if (!bdev) {
        DRV_LOG_INIT(ERR, "Could not create bdev");
        goto undo_blkdev_reg;
    }

    add_disk(bdev->gd);
    DRV_LOG_INIT(INFO, "Successfully initialized");
    return DRV_OP_SUCCESS;

undo_blkdev_reg:
    unregister_blkdev(major, DRV_NAME);
err:
    return -EBUSY;
}


static void __exit drv_exit(void) {}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
