#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>

#define DRV_NAME "memes"
#define DRV_LOG_DELIM ": "

// General purpose log prefix
#define DRV_LOG_NAME "driver"
#define DRV_LOG(type, ctx, ...) \
    do { \
        printk(KERN_##type \
               DRV_LOG_NAME \
               DRV_LOG_DELIM \
               ctx \
               DRV_LOG_DELIM \
               __VA_ARGS__); \
    } while(0)

