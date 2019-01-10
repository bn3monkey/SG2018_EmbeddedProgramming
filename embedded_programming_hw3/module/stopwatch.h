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
#include <linux/semaphore.h> // for semaphore

#include <linux/cdev.h> //for character device

#include <mach/gpio.h>
#include <asm/gpio.h> // for gpio

#include <asm/irq.h> // for irq

#include <linux/interrupt.h> // for interrupt

#include <linux/wait.h> // for make main thread wait

#define STOPWATCH_MAJOR 242 
#define STOPWATCH_MINOR 0
#define STOPWATCH_NAME "stopwatch"

int __init stopwatch_init(void);
void __exit stopwatch_exit(void);
int stopwatch_open(struct inode *, struct file *);
int stopwatch_release(struct inode *, struct file *);
ssize_t stopwatch_write(struct file *, const char *, size_t, loff_t *);

irqreturn_t stopwatch_start(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t stopwatch_pause(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t stopwatch_reset(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t stopwatch_end(int irq, void* dev_id, struct pt_regs* reg);


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

#endif