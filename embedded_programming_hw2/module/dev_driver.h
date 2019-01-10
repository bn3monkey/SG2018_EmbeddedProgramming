#ifndef __DEV_DRIVER__
#define __DEV_DRIVER__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/timer.h> // for timer
#include <linux/errno.h> //for error codes
#include <linux/slab.h> // for kmalloc

#include <linux/fs.h> // for file operation

#include <linux/uaccess.h> // for access user memory

#include <linux/io.h>
#include <linux/semaphore.h>

#define MYDEV_DRIVER_MAJOR 242 
#define MYDEV_DRIVER_MINOR 0
#define MYDEV_DRIVER_NAME "dev_driver"

int __init mydev_driver_init(void);
void __exit mydev_driver_exit(void);
int mydev_driver_open(struct inode *, struct file *);
int mydev_driver_release(struct inode *, struct file *);
ssize_t mydev_driver_write(struct file *, const char *, size_t, loff_t *);
int mydev_driver_ioctl(struct file* flip, unsigned int cmd, unsigned long arg);

inline void direct_write(void* dest, void* src, int size)
{
    unsigned char* out = (unsigned char *)dest;
    unsigned char* in = (unsigned char *)src;

    while(size--)
    {
        *out = *in;
        out++;
        in++;
    }
}
inline void direct_dot_write(void* dest, unsigned char *src, int size)
{
    int i;
	unsigned short _s_value = 0;
	for(i=0;i<size;i++)
    {
        _s_value = src[i] & 0x7F;
		outw(_s_value,(unsigned int)dest+i*2);
    }
}
inline void direct_text_write(void* dest, unsigned char *src, int size)
{
    int i;
	unsigned short _s_value = 0;
    
    src[size] = '\0';

	for(i=0;i<size;i++)
    {
        _s_value = ((src[i] & 0xFF) << 8) | (src[i + 1] & 0xFF);
		outw(_s_value,(unsigned int)dest+i);
		i++;
    }
}
#endif