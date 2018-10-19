#include "commands.h"
#include "constants.h"


/* Module scope variables */
static struct {
    dev_t dev;
    struct cdev cdev;
    struct class* cl;
    struct file* workfile_fp;
} mscope = {
    .workfile_fp = NULL
};


static int chdev_open(struct inode *ip, struct file *fp)
{
    return DRV_SUCCESS;
}


static int chdev_release(struct inode *ip, struct file *fp)
{
    return DRV_SUCCESS;
}


static ssize_t chdev_read(struct file *fp, 
                          char __user *buf, 
                          size_t len, 
                          loff_t* off)
{
    return DRV_SUCCESS;
}


// Log message prefixes
#define OPEN_LOG_PREFIX "open(): "
#define OPEN_CMD_INFO DRV_LOG_WR_INFO OPEN_LOG_PREFIX
#define OPEN_CMD_ERR DRV_LOG_WR_INFO OPEN_LOG_PREFIX 
#define OPEN_CMD_DBG DRV_LOG_WR_DBG OPEN_LOG_PREFIX 

#define CLOSE_LOG_PREFIX "close(): "
#define CLOSE_CMD_INFO DRV_LOG_WR_INFO CLOSE_LOG_PREFIX 
#define CLOSE_CMD_ERR DRV_LOG_WR_INFO CLOSE_LOG_PREFIX 
#define CLOSE_CMD_DBG DRV_LOG_WR_DBG CLOSE_LOG_PREFIX 

#define WRITE_LOG_PREFIX "write(): "
#define WRITE_CMD_INFO DRV_LOG_WR_INFO WRITE_LOG_PREFIX 
#define WRITE_CMD_ERR DRV_LOG_WR_INFO WRITE_LOG_PREFIX 
#define WRITE_CMD_DBG DRV_LOG_WR_DBG WRITE_LOG_PREFIX 


static bool str_is_empty(char const* str)
{
    return str[0] == '\0';
}


static bool cmd_open(char const* fname)
{
    if (mscope.workfile_fp) {
        printk(OPEN_CMD_ERR "some file is already opened\n");
        return false;
    }
    mscope.workfile_fp = kfile_open(fname, DRV_WORKFILE_PERM);
    return mscope.workfile_fp != NULL;
}


static bool cmd_close(void) 
{
    if (!mscope.workfile_fp) {
        printk(CLOSE_CMD_ERR "file is not open");
        return false;
    }
    kfile_close(mscope.workfile_fp);
    mscope.workfile_fp = NULL;
    return true;
}


static bool cmd_write(char const *buf, size_t sz, loff_t *off)
{
    ssize_t wrote = 0;
    if (!mscope.workfile_fp) {
        printk(WRITE_CMD_ERR "file is not opened");
        return false;
    }
    wrote = kfile_write(mscope.workfile_fp, off, buf, sz);
    printk(WRITE_CMD_DBG "size: %lu; wrote: %li\n", sz, wrote);
    return wrote == sz;
}


static ssize_t chdev_write(struct file *fp, 
                           const char __user *buf, 
                           size_t len, 
                           loff_t *off)
{
    char *str = NULL;

    printk(DRV_LOG_WR_DBG "buffer size %lu\n", len);
    printk(DRV_LOG_WR_DBG "offset %lli\n", *off);
    printk(DRV_LOG_WR_DBG "moving incoming buf to kernel space\n");

    str = kmalloc(len, GFP_KERNEL);
    copy_from_user(str, buf, len);
    str[len - 1] = '\0';

    printk(DRV_LOG_WR_DBG "Buffer successfully moved. Content %s\n", str);

    if (str_is_empty(str)) {
        printk(DRV_LOG_WR_INFO "No operations\n");

    } else if (str_contains(str, DRV_CMD_OPEN)) {
        char const *fname = str + DRV_CMD_OPEN_STRLEN;
        printk(OPEN_CMD_INFO "Performing open() command\n");
        if (str_is_empty(fname))
            printk(OPEN_CMD_ERR "failed. Filename is empty.\n");
        else if (!cmd_open(fname))
            printk(OPEN_CMD_ERR "failed. Could not open the file \"%s\".\n", fname);
        else
            printk(OPEN_CMD_INFO "operation successfully performed\n");

    } else if (strcmp(str, DRV_CMD_CLOSE) == 0) {
        printk(CLOSE_CMD_INFO "Performing close() command\n");
        if (!cmd_close())
            printk(CLOSE_CMD_ERR "failed. Could not close the file\n");
        else 
            printk(CLOSE_CMD_INFO "operation successfully performed\n");

    } else {
        printk(WRITE_CMD_INFO "Perfrorming write() command\n");
        if (!cmd_write(str, len, off))
            printk(WRITE_CMD_ERR "failed. Could not write to file\n");
        else
            printk(WRITE_CMD_INFO "operation successfully performed\n");
    }

    kfree(str);
    return len;
}


static const struct file_operations chdev_ops = {
    .owner = THIS_MODULE,
    .open = chdev_open,
    .release = chdev_release,
    .read = chdev_read,
    .write = chdev_write
};


static int __init chdev_init(void)
{
    int err = DRV_FAILURE;

    if (( err = alloc_chrdev_region(&mscope.dev, 0, 1, "lab1_chdrv") )) {
        printk(DRV_LOG_ERR "Failed to alloc region. Error code: %i", err);
        return err;
    } 
    if ((mscope.cl = class_create(THIS_MODULE, "ch_driver")) == NULL) {
        printk(DRV_LOG_ERR "Failed to create class. Error code: %i\n", err);
        goto err_undo_reg;
    }
    if (device_create(mscope.cl, NULL, mscope.dev, NULL, "chdev") == NULL) {
        printk(DRV_LOG_ERR "Failed to create device. Error code: %i\n", err);
        goto err_undo_cl_create;
    }

    cdev_init(&mscope.cdev, &chdev_ops);

    if (( err = cdev_add(&mscope.cdev, mscope.dev, 1) )) {
        printk(DRV_LOG_ERR "Failed to add cdev. Error code: %i\n", err);
        goto err_undo_dev_create;
    }

    printk(DRV_LOG_INFO "Module successfully loaded\n");
    return DRV_SUCCESS;

err_undo_dev_create: 
    device_destroy(mscope.cl, mscope.dev);
    printk(DRV_LOG_DBG "Device destoryed\n");
err_undo_cl_create: 
    class_destroy(mscope.cl);
    printk(DRV_LOG_DBG "Class destoryed\n");
err_undo_reg: 
    unregister_chrdev_region(mscope.dev, 1);
    printk(DRV_LOG_DBG "Region unregistered\n");
     
    return err;
}


static void __exit chdev_exit(void)
{
    cdev_del(&mscope.cdev);
    device_destroy(mscope.cl, mscope.dev);
    class_destroy(mscope.cl);
    unregister_chrdev_region(mscope.dev, 1);

    printk(DRV_LOG_INFO "Module has removed\n");
}


MODULE_LICENSE("GPL");
module_init(chdev_init);
module_exit(chdev_exit);