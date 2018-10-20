#include "commands.h"


bool str_contains(const char *str, const char* substr)
{
    return strstr(str, substr) != NULL;
}


char* str_copy_ks(char *kstr, const char __user *buf, size_t buflen)
{
    copy_from_user(kstr, buf, buflen);
    kstr[buflen-1] = '\0';
    return kstr;
}


struct file *kfile_open(const char *path, mode_t mode)
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, O_RDWR | O_CREAT, mode);
    set_fs(oldfs);

    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}


void kfile_close(struct file *fp)
{
    filp_close(fp, NULL);
}


ssize_t kfile_write(struct file *file, 
                    unsigned long long* offset, 
                    const char *data, 
                    size_t size)
{
    mm_segment_t oldfs;
    int ret = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    ret = vfs_write(file, data, size, offset);
    set_fs(oldfs);

    return ret;
}


ssize_t kfile_read(struct file *file, 
                   unsigned long long* offset, 
                   char *data, 
                   size_t size)
{
    mm_segment_t oldfs;
    int ret = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    ret = vfs_read(file, data, size, offset);
    set_fs(oldfs);

    return ret;
}