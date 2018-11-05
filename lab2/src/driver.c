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


static struct
{
    struct drv_blkdev_geo disk_geo;
    struct drv_blkdev * blkdev;
    int major;
    char const * device_name;
} module_scope
    = {.disk_geo = {.nsectors = DRV_NSECTORS, .sector_sz = DRV_SECTOR_SZ},
       .device_name = DRV_NAME,
       .major = 0};

static struct block_device_operations blk_ops = {.owner = THIS_MODULE};

static void drv_request(struct request_queue * queue) {}


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
    struct drv_blkdev * bdev = NULL;

    DRV_LOG_INIT(DEBUG, "Alloc mem for blkdev struct\n");
    bdev = kmalloc(sizeof(struct block_device), GFP_KERNEL);
    if (!bdev) {
        DRV_LOG_INIT(ERR, "Failed to alloc memory for block_device\n");
        goto out;
    }

    memset(bdev, 0, sizeof(struct block_device));
    bdev->size = geo.nsectors * geo.sector_sz;
    bdev->vdisk = vmalloc(bdev->size);

    DRV_LOG_INIT(DEBUG, "Alloc mem for vritual disk\n");
    if (bdev->vdisk == NULL) {
        DRV_LOG_INIT(ERR, "Failed to alloc memory for virtual disk\n");
        goto undo_bdev_alloc;
    }

    DRV_LOG_INIT(DEBUG, "Create device's request queue\n");
    spin_lock_init(&bdev->lock);
    bdev->queue = blk_init_queue(drv_request, &bdev->lock);
    if (!bdev->queue) {
        DRV_LOG_INIT(ERR, "Failed to allocate memory for blk queue\n");
        goto undo_bdisk_alloc;
    }

    DRV_LOG_INIT(DEBUG, "Create gendisk structure\n");
    bdev->gd = gendisk_create(bdev, &blk_ops, major, minors);
    if (!bdev->gd) {
        DRV_LOG_INIT(ERR, "Failed to allocate gendisk\n");
        goto undo_init_queue;
    }

    DRV_LOG_INIT(INFO, "Block device successfully created\n");
    return bdev;

undo_init_queue:
    blk_cleanup_queue(bdev->queue);
undo_bdisk_alloc:
    vfree(bdev->vdisk);
undo_bdev_alloc:
    kfree(bdev);
out:
    return NULL;
}


static int __init drv_init(void)
{
    DRV_LOG_INIT(INFO, "Starting initialization\n");
    DRV_LOG_INIT(DEBUG, "Register block device");
    module_scope.major = register_blkdev(module_scope.major, DRV_NAME);

    if (module_scope.major <= 0) {
        DRV_LOG_INIT(ERR, "Could not register block device\n");
        goto err;
    }

    DRV_LOG_INIT(DEBUG, "Create block device");
    module_scope.blkdev
        = bdev_create(module_scope.major, DRV_MINORS, module_scope.disk_geo);

    if (!module_scope.blkdev) {
        DRV_LOG_INIT(ERR, "Could not create bdev\n");
        goto undo_blkdev_reg;
    }

    DRV_LOG_INIT(DEBUG, "Add disk to the system");
    add_disk(module_scope.blkdev->gd);
    DRV_LOG_INIT(INFO, "Successfully initialized\n");
    return DRV_OP_SUCCESS;

undo_blkdev_reg:
    unregister_blkdev(module_scope.major, DRV_NAME);
err:
    return -EBUSY;
}


static void __exit drv_exit(void)
{
    struct drv_blkdev * blkdev = module_scope.blkdev;
    del_gendisk(blkdev->gd);
    put_disk(blkdev->gd);
    unregister_blkdev(module_scope.major, module_scope.device_name);
    blk_cleanup_queue(blkdev->queue);
    vfree(blkdev->vdisk);
    kfree(blkdev);
    module_scope.blkdev = NULL;
}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
