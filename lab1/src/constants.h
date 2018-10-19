#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>

#define DRV_LOG_DELIM ": "

// General purpose log prefix
#define DRV_LOG_NAME "driver"
#define DRV_LOG_INFO KERN_INFO DRV_LOG_NAME DRV_LOG_DELIM
#define DRV_LOG_ERR KERN_ERR DRV_LOG_NAME DRV_LOG_DELIM 
#define DRV_LOG_DBG KERN_DEBUG DRV_LOG_NAME DRV_LOG_DELIM 

#define DRV_LOG_OP(type, op) DRV_LOG_##type DRV_LOG_##op##_OP DRV_LOG_DELIM 

// Log prefix for write() call
#define DRV_LOG_WR_OP "write"
#define DRV_LOG_WR_INFO  DRV_LOG_OP(INFO, WR)
#define DRV_LOG_WR_ERR   DRV_LOG_OP(ERR, WR)
#define DRV_LOG_WR_DBG   DRV_LOG_OP(DBG, WR)

#define DRV_SUCCESS  0
#define DRV_FAILURE -1
#define DRV_INVALID_FD -1

#define DRV_CMD_OPEN "open "
#define DRV_CMD_OPEN_STRLEN (sizeof(DRV_CMD_OPEN) - 1)

#define DRV_CMD_CLOSE "close"
#define DRV_CMD_CLOSE_STRLEN (sizeof(DRV_CMD_CLOSE) - 1))

#define DRV_WORKFILE_PERM 0666

