#ifndef CDEV_COMMANDS_H
#define CDEV_COMMANDS_H

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


struct file *kfile_open(const char *fname, mode_t mode);
void kfile_close(struct file *fp);
ssize_t kfile_write(struct file *file, unsigned long long* offset, const char *data, size_t size);
ssize_t kfile_read(struct file *file, unsigned long long offset, char *data, size_t size);

char* str_copy_ks(char *kstr, const char __user *buf, size_t buflen);
bool str_contains(const char *str, const char* substr);

#endif