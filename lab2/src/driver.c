#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/buffer_head.h>
#include <asm/uaccess.h>



static int __init drv_init(void)
{

}

static void __exit drv_exit(void)
{

}


MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);