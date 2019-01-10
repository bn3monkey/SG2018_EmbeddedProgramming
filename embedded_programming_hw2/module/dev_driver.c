#include "dev_driver.h"
#include "dev_step.c"

#define DEV_IOCTL_MAGIC 'S'
#define DEV_IOCTL_WRITE _IOW(DEV_IOCTL_MAGIC, 0, int)

static struct file_operations mydev_driver_fops =
{
    .open = mydev_driver_open, 
    .write = mydev_driver_write,
	.release = mydev_driver_release,
	.unlocked_ioctl = mydev_driver_ioctl,
};

struct timer_list timer;

//paramter when using write method
static struct mydev_parameter
{
	unsigned char fnd_index;
	unsigned char fnd_value;
	unsigned char gap;
	unsigned char times;
} param;

//output value when direct write to physical memory
#define fnd_buflen 2
#define led_buflen 1
#define text_buflen 32
#define dot_buflen 10

static struct mydev_buffer
{
	unsigned char fnd[fnd_buflen + 1];
	unsigned char led[led_buflen + 1];
	unsigned char text[text_buflen + 5];
	unsigned char dot[dot_buflen + 1];
} buf;

enum mem_addr
{
	FND_ADDR = 0x4,
	LED_ADDR = 0x16,
	TEXT_ADDR = 0x90,
	DOT_ADDR = 0x210,
	BASE_ADDR = 0x08000000
};
static struct mydev_memory
{
	void __iomem *base;
	void __iomem *fnd;
	void __iomem *led;
	void __iomem *text;
	void __iomem *dot;
} mem;

struct semaphore write_lock;

//prevent from double open
static int driver_usage;
//prevent from double write before previous write function ends
static int driver_write_usage; 

int mydev_driver_open(struct inode *minode, struct file *mfile)
{
	printk("%s open\n",MYDEV_DRIVER_NAME);
	
	//1. if already open, cancel to open
	if ( driver_usage != 0)
	{
		printk("Fail to open %s : already opened!\n",MYDEV_DRIVER_NAME);
		return -EBUSY;
	}

	//2. physical memory mapping
	mem.base = ioremap(BASE_ADDR, 0x1000);
	mem.fnd = mem.base + FND_ADDR;
	mem.led = mem.base + LED_ADDR;
	mem.dot = mem.base + DOT_ADDR;
	mem.text = mem.base + TEXT_ADDR;
	
	printk("address : %p %p %p %p\n",mem.fnd, mem.led, mem.dot, mem.text);

	//3. mutex initialization
	sema_init(&write_lock, 0);

	driver_usage = 1;
	driver_write_usage = 0;
	return 0;
}
int mydev_driver_release(struct inode *minode, struct file *mfile)
{
	
	//1. physical memory unmapping
	iounmap(mem.base);
	mem.fnd = NULL;
	mem.led = NULL;
	mem.dot = NULL;
	mem.text = NULL;
	
	//2. reset usage variable
	driver_usage = 0;
	driver_write_usage = 0;

	printk("%s release\n",MYDEV_DRIVER_NAME);
	return 0;
}

static void mydev_driver_callback(unsigned long _param)
{
	//1. get parameter
	struct mydev_parameter* tmp_param = (struct mydev_parameter*)_param;
	//static unsigned char count = 0;

	//2. if the number of times becomes 0, stop callback
	if (tmp_param->times-- == 0)
	{
		driver_write_usage = 0;

		//2-1. fnd initialization
		buf.fnd[0] = 0;
		buf.fnd[1] = 0;
		direct_write(mem.fnd, buf.fnd, fnd_buflen);
		
		//2-2. led initialization
		buf.led[0] = 0;
		direct_write(mem.led, buf.led, led_buflen);

		//2-3. dot initialization
		dot_off(buf.dot);
		direct_dot_write(mem.dot, buf.dot, dot_buflen);

		//2-4. text initialization
		text_off(buf.text);
		direct_text_write(mem.text, buf.text, text_buflen);

		//write function(main thread) make waiting end.
		up(&write_lock);
		return;
	}
	printk("now times : %d\n",(int)tmp_param->times);


	//3. set value of buffers
	direct_write(mem.fnd, buf.fnd, fnd_buflen);
	fnd_step(buf.fnd);

	direct_write(mem.led, buf.led, led_buflen);
	led_step(buf.led);

	direct_dot_write(mem.dot, buf.dot, dot_buflen);
	dot_step(buf.dot);
		
	direct_text_write(mem.text, buf.text, text_buflen);
	text_step(buf.text);

	//4. set next callback
	timer.expires = get_jiffies_64() + (HZ/10) * (int)param.gap;
	timer.data = (unsigned long)(&param);
	timer.function = mydev_driver_callback;

	add_timer(&timer);
}

ssize_t mydev_driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
	struct mydev_parameter* tmp_param = (struct mydev_parameter* )gdata;
	
	printk("%s write\n", MYDEV_DRIVER_NAME);
	//1. if write before open, end this function
	if(driver_usage == 0)
	{
		printk("Fail to Write : device doesn't open!\n");
		return -EFAULT;
	}

	//2. if write function already running, end this function
	if(driver_write_usage > 0)
	{
		printk("Fail to Write : already writing!\n");
		return -EFAULT;
	}
	driver_write_usage = 1;

	//3. copy paramter from user memory to kernel memory
	if (copy_from_user(&param, tmp_param, sizeof(struct mydev_parameter)))
	{
		printk("Fail to Write : ncopy_from_user fail!\n");
		return -EFAULT;
	}
	printk("Timer parameter Setting (%d %c %d %d)\n",(int)param.fnd_index, param.fnd_value, (int)param.gap, (int)param.times 
		);

	//4. set timer structure
	timer.expires = get_jiffies_64() + (HZ/10) * (int)param.gap;
	timer.data = (unsigned long)(&param);
	timer.function = mydev_driver_callback;
	
	//5. set first value of buffers
	fnd_init_step(param.fnd_index, param.fnd_value, buf.fnd);
	led_init_step(param.fnd_value, buf.led);
	dot_init_step(param.fnd_value, buf.dot);
	text_init_step(buf.text);

	//6. delete previous timer and add new timer
	del_timer_sync(&timer);
	add_timer(&timer);

	//7. if write is running, waits
	down(&write_lock);

	return 0;
}

int mydev_driver_ioctl(struct file* flip, unsigned int cmd, unsigned long arg)
{
	printk ("%s ioctl!\n", MYDEV_DRIVER_NAME);
	switch(cmd)
	{
		case DEV_IOCTL_WRITE :
			printk("%s write driver call!\n", MYDEV_DRIVER_NAME);
			mydev_driver_write(NULL, (const char *)arg, 0, NULL);
			break;
	}
	return 1;
}

int __init mydev_driver_init(void)
{
	int result;


	printk("%s init\n", MYDEV_DRIVER_NAME);

    //1. Register driver
	result = register_chrdev(MYDEV_DRIVER_MAJOR, MYDEV_DRIVER_NAME, &mydev_driver_fops);
	if(result <0) {
		printk( "Fail to register : dev_name : %s result : %d\n",
            MYDEV_DRIVER_NAME, result);
		return result;
	}
    printk( "Success to register : dev_file : /dev/%s , major : %d\n", 
        MYDEV_DRIVER_NAME, MYDEV_DRIVER_MAJOR);

	//2. initialize timer
	init_timer(&timer);

	printk("Success to initialize\n");
	return 0;
}

void __exit mydev_driver_exit(void)
{
	printk("%s exit\n",MYDEV_DRIVER_NAME);

	//1. close driver
	driver_usage = 0;
	driver_write_usage = 0;

    //2. unregister driver.
	unregister_chrdev(MYDEV_DRIVER_MAJOR, MYDEV_DRIVER_NAME);

	//3. delete timer. (Waiting for handler function by using _sync)
	del_timer_sync(&timer);

    printk("Success to exit\n");
}

module_init( mydev_driver_init);
module_exit( mydev_driver_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("bn3monkey");
