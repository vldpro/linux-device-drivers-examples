#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#define DRV_LOG_DISABLE_DEBUG

#include "constants.h"
#include "logging.h"


struct drv_blkdev
{
    int minors;
    struct gendisk * gd;
    struct request_queue * queue;
    char * vdisk;
    size_t size;
    spinlock_t lock;
};

static struct
{
    int blk_major;
    struct drv_blkdev blkdev;
    struct block_device_operations blk_ops;
} module_globals = {.blk_major = 0,
                    .blkdev = {.vdisk = NULL,
                               .queue = NULL,
                               .size = 0,
                               .gd = NULL,
                               .minors = DRV_MINORS},
                    .blk_ops = {.owner = THIS_MODULE}};


int drv_ioctl(struct inode * inode,
              struct file * filp,
              unsigned int cmd,
              unsigned long arg)
{
    long size;
    struct hd_geometry geo;
    DRV_LOG_CTX_SET("drv_ioctl");

    switch (cmd) {
            /*
	 * The only command we need to interpret is HDIO_GETGEO, since
	 * we can't partition the drive otherwise.  We have no real
	 * geometry, of course, so make something up.
	 */
        case HDIO_GETGEO:
            LG_DBG("retreiving disk geo");
            size = module_globals.blkdev.size
                   * (DRV_SECTOR_SZ / KERNEL_SECTOR_SIZE);
            geo.cylinders = (size & ~0x3f) >> 6;
            geo.heads = 4;
            geo.sectors = 16;
            geo.start = 4;

            if (copy_to_user((void *)arg, &geo, sizeof(geo)))
                return -EFAULT;
            return 0;
    }

    LG_DBG("unknown commad");
    return -ENOTTY; /* unknown command */
}


static int drv_transfer(struct drv_blkdev * blkdev,
                        sector_t sector,
                        size_t nsect,
                        char * buf,
                        int write)
{
    size_t off = sector * DRV_SECTOR_SZ;
    size_t nbytes = nsect * DRV_SECTOR_SZ;
    DRV_LOG_CTX_SET("drv_transfer");

#ifndef DRV_LOG_DISABLE_DEBUG
    printk(KERN_DEBUG "drv_transfer: params: off: %lu, nbytes: %lu\n",
           off,
           nbytes);

    if (write)
        LG_DBG("start writing");
    else
        LG_DBG("start reading");
#endif

    if ((off + nbytes) > blkdev->size) {
        LG_FAILED_TO("write to / read from device. Out of bound.");
        return -ENOSPC;
    }

    if (write)
        memcpy(blkdev->vdisk + off, buf, nbytes);
    else
        memcpy(buf, blkdev->vdisk + off, nbytes);

    return 0;
}


static void drv_request_handler(struct request_queue * queue)
{
    struct request * rq = NULL;
    struct drv_blkdev * blkdev = queue->queuedata;
    int blk_op_status = 0;

    DRV_LOG_CTX_SET("drv_request_handler");

    rq = blk_fetch_request(queue);
    for (;;) {
        if (rq == NULL)
            break;

        if (rq->cmd_type != REQ_TYPE_FS) {
            LG_WRN("Skip non-fs request");
            __blk_end_request_all(rq, -EIO);
            continue;
        }

        blk_op_status = drv_transfer(blkdev,
                                     blk_rq_pos(rq),
                                     blk_rq_cur_sectors(rq),
                                     bio_data(rq->bio),
                                     rq_data_dir(rq));

        if (!__blk_end_request_cur(rq, blk_op_status))
            rq = blk_fetch_request(queue);
    }
}


static int drv_gendisk_create(struct drv_blkdev * blkdev)
{
    DRV_LOG_CTX_SET("drv_gendisk_create");

    LG_DBG("Alloc gendisk");
    blkdev->gd = alloc_disk(blkdev->minors);
    if (!blkdev->gd)
        return -ENOMEM;

    LG_DBG("Initialize gendisk");
    blkdev->gd->major = module_globals.blk_major;
    blkdev->gd->first_minor = 0;
    blkdev->gd->fops = &module_globals.blk_ops;
    blkdev->gd->queue = blkdev->queue;
    blkdev->gd->private_data = blkdev;

    snprintf(blkdev->gd->disk_name, DRV_DISKNAME_MAX, DRV_NAME);

    LG_DBG("Adding gendisk into the system");

    // See https://stackoverflow.com/questions/13518404/add-disk-hangs-on-insmod
    set_capacity(blkdev->gd, 0);
    add_disk(blkdev->gd);
    set_capacity(blkdev->gd,
                 DRV_NSECTORS * (DRV_SECTOR_SZ / KERNEL_SECTOR_SIZE));

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

    LG_DBG("Allocate memory for vdisk");
    blkdev->size = DRV_SECTOR_SZ * DRV_NSECTORS;
    blkdev->vdisk = vmalloc(blkdev->size);
    if (blkdev->vdisk == NULL) {
        LG_FAILED_TO("allocate memory for vdisk");
        goto out;
    }

    LG_DBG("Initialize queue");
    spin_lock_init(&blkdev->lock);
    blkdev->queue = blk_init_queue(drv_request_handler, &blkdev->lock);
    if (!blkdev->queue) {
        LG_FAILED_TO("initialize requests queue");
        goto undo_vdisk_alloc;
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
undo_vdisk_alloc:
    vfree(blkdev->vdisk);
out:
    blkdev->vdisk = NULL;
    blkdev->size = 0;
    return -ENOMEM;
}


static void drv_blkdev_deinit(struct drv_blkdev * blkdev)
{
    drv_gendisk_delete(blkdev->gd);
    blk_cleanup_queue(blkdev->queue);
    vfree(blkdev->vdisk);

    blkdev->gd = NULL;
    blkdev->queue = NULL;
    blkdev->vdisk = NULL;
    blkdev->size = 0;
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

    LG_INF("Module successfully initialized");
    return DRV_OP_SUCCESS;

undo_blkdev_reg:
    unregister_blkdev(module_globals.blk_major, DRV_NAME);
out:
    return status;
}


static void __exit drv_exit(void)
{
    DRV_LOG_CTX_SET("drv_exit");
    drv_blkdev_deinit(&module_globals.blkdev);
    unregister_blkdev(module_globals.blk_major, DRV_NAME);
    LG_DBG("Module was removed");
}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);
