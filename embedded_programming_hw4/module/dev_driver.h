#ifndef __DEV_DRIVER__
#define __DEV_DRIVER__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/timer.h> // for timer
#include <linux/errno.h> //for error codes
#include <linux/slab.h> // for kmalloc

#include <linux/fs.h> // for file operation

#include <linux/io.h>

#include <linux/cdev.h> //for character device

#include <mach/gpio.h>
#include <asm/gpio.h> // for gpio

#include <asm/irq.h> // for irq

#include <linux/interrupt.h> // for interrupt

#include <linux/wait.h> // for make main thread wait

#include <asm/uaccess.h> //for copy from/to user memory

#define driver_MAJOR 242 
#define driver_MINOR 0
#define driver_NAME "dev_driver"

int __init driver_init(void);
void __exit driver_exit(void);
int driver_open(struct inode *, struct file *);
int driver_release(struct inode *, struct file *);
ssize_t driver_read(struct file *, char __user *, size_t, loff_t *);
ssize_t driver_write(struct file *, const char *, size_t, loff_t *);
int driver_ioctl(struct file* flip, unsigned int cmd, unsigned long arg);

irqreturn_t mode_change(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t timer_setup(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t timer_start(int irq, void* dev_id, struct pt_regs* reg);

void write_to_fnd(unsigned long second);
void write_to_dot(unsigned long second);

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

unsigned char fpga_number[10][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};
unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#define DEV_IOCTL_MAGIC 'S'
#define DEV_IOCTL_WRITE _IOW(DEV_IOCTL_MAGIC, 0, int)

#endif