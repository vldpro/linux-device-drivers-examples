#ifndef LOGGING_H
#define LOGGING_H


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#define DRV_NAME "memes"
#define DRV_LOG_DELIM ": "

// General purpose log
#define DRV_LOG_NAME "driver"
#define DRV_LOG(type, ctx, ...)                                          \
    do {                                                                 \
        printk(KERN_##type DRV_LOG_NAME DRV_LOG_DELIM DRV_LOG_DELIM ctx, \
               __VA_ARGS__);                                             \
    } while (0)

#define DRV_LOG_INIT(type, ...) DRV_LOG(type, "init", __VA_ARGS__)

// Contextual logging
#define DRV_LOG_CTX_TYPE_SET(ctx, type)        \
    static char const * _hidden_log_ctx_##type \
        = KERN_##type DRV_LOG_NAME DRV_LOG_DELIM ctx DRV_LOG_DELIM "%s\n";

#define DRV_LOG_CTX_SET(ctx)         \
    DRV_LOG_CTX_TYPE_SET(ctx, INFO)  \
    DRV_LOG_CTX_TYPE_SET(ctx, DEBUG) \
    DRV_LOG_CTX_TYPE_SET(ctx, ERR)   \
    DRV_LOG_CTX_TYPE_SET(ctx, WARNING)

#define DRV_LOG_CTX(type, log_msg)               \
    do {                                         \
        printk(_hidden_log_ctx_##type, log_msg); \
    } while (0)

// user's logging functions
#define LG_INF(log_msg) DRV_LOG_CTX(INFO, log_msg)
#define LG_ERR(log_msg) DRV_LOG_CTX(ERR, log_msg)
#define LG_DBG(log_msg) DRV_LOG_CTX(DEBUG, log_msg)
#define LG_WRN(log_msg) DRV_LOG_CTX(WARNING, log_msg)
#define LG_FAILED_TO(action) LG_ERR("Failed to " action)

#endif
