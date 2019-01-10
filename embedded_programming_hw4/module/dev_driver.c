#include "dev_driver.h"

static struct file_operations driver_fops =
{
    .open = driver_open,
	.read = driver_read, 
    .write = driver_write,
	.release = driver_release,
	.unlocked_ioctl = driver_ioctl
};
struct timer_list timer;

enum mem_addr
{
	FND_ADDR = 0x4,
	DOT_ADDR = 0x210,
	BASE_ADDR = 0x08000000
};
static struct driver_memory
{
	void __iomem *base;
	void __iomem *fnd;
	void __iomem *dot;
} mem;

#define fnd_buflen 2
unsigned char fnd[fnd_buflen + 1];
#define dot_buflen 10
unsigned char dot[dot_buflen + 1];

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

//prevent from double open
static int driver_usage;
//prevent from double read
static int driver_read_usage;
//prevent from double write
static int driver_write_usage;
//prevent from double ioctl
static int driver_ioctl_usage;
//prevent from double start
static int timer_start_usage;
//change the toggle
static int mode_change_toggle = 0;
static unsigned long timer_second = 0;
static unsigned long counter_count = 0;

void write_to_fnd(unsigned long second)
{
	int minute = (second/100)%10000;
	fnd[0] = ((second/10)<<4) + second % 10;
	fnd[1] = ((minute/10)<<4) + minute % 10;
	direct_write(mem.fnd, fnd, fnd_buflen);
}
void write_to_dot(unsigned long second)
{
	int i;
	//1. if second is not 0~9, make blank
	if(second  >= 10)
		for(i=0;i<10;i++)
        	dot[i] = fpga_set_blank[i];
	//2. or not, make number
	else
		for(i=0;i<10;i++)
        	dot[i] = fpga_number[second][i];

	direct_dot_write(mem.dot, dot, dot_buflen);
}

static void timer_tick(unsigned long second)
{
	//if times out
	if(second == 0)
	{
		//1. device initialzation
		write_to_fnd(0);
		write_to_dot(10);
		//2. wrtie toggle change
		timer_start_usage = 0;
		//3. wakes thread calling write
		__wake_up(&wq_write, 1, 1, NULL);
		
		return;
	}
	printk("NOW TIME (%lu)\n", second);
	//1. write to fnd
	write_to_fnd(second);
	//2. wrtie to dot
	write_to_dot(second);
	
	//3. add next timer
	timer.expires = get_jiffies_64() + HZ;
	timer.data = timer_second = second - 1;
	timer.function = timer_tick;

	add_timer(&timer);
}


irqreturn_t mode_change(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(MODE_CHANGE) : %d\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

	//1. if timer is running , do not change mode
	if(timer_start_usage)
	{
		printk(KERN_ALERT "timer is running\n");
		return IRQ_HANDLED;
	}
	//2. mode_change_toggle conversion
	mode_change_toggle = 1 - mode_change_toggle;
	if(mode_change_toggle)
	{
		//timer
		timer_second = 0;
		write_to_fnd(0);
		write_to_dot(10);
	}
	else
	{
		//counter
		write_to_fnd(counter_count);
		write_to_dot(counter_count % 10);
	}
	printk(KERN_ALERT "NOW MODE : %s\n",mode_change_toggle ? "TIMER" : "COUNTER");
    return IRQ_HANDLED;
}
irqreturn_t timer_setup(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(SETUP) : %d\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	//1. if mode is timer?
	if(!mode_change_toggle)
	{
		printk(KERN_ALERT "mode is in counter\n");
		return IRQ_HANDLED;
	}
	//2. already timer starts?
	if(timer_start_usage)
	{
		printk(KERN_ALERT "timer is running\n");
		return IRQ_HANDLED;
	}
	//3. timer configuration
	printk("NOW TIME (%lu)\n", timer_second);
	timer_second = (timer_second+1) % 60;
	write_to_fnd(timer_second);
	write_to_dot(timer_second);

	return IRQ_HANDLED;
}

irqreturn_t timer_start(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(start) : %d\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
	//1. if mode is timer?
	if(!mode_change_toggle)
	{
		printk(KERN_ALERT "mode is in counter\n");
		return IRQ_HANDLED;
	}
	//2. already timer starts?
	if(timer_start_usage)
	{
		printk(KERN_ALERT "timer is running\n");
		return IRQ_HANDLED;
	}
	//3. timer starts!
	timer_start_usage = 1;

	//4. add timer
	timer.expires = get_jiffies_64() + HZ;
	timer.data = timer_second;
	timer.function = timer_tick;

	add_timer(&timer);
	return IRQ_HANDLED;
}


static int request_interrupt(void)
{
	int ret;
	int irq;

	printk(KERN_ALERT "Request interrupt\n");

	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, mode_change, IRQF_TRIGGER_FALLING, "home", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, timer_setup, IRQF_TRIGGER_FALLING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, timer_start, IRQF_TRIGGER_FALLING, "voldown", 0);

	return 0;
}

int driver_open(struct inode *minode, struct file *mfile)
{
    printk("%s open\n",driver_NAME);
    //1. if already open, cancel to open
	if ( driver_usage != 0)
	{
		printk("Fail to open %s : already opened!\n",driver_NAME);
		return -EBUSY;
	}

    //2. physical memory mapping
	mem.base = ioremap(BASE_ADDR, 0x1000);
	mem.fnd = mem.base + FND_ADDR;
	mem.dot = mem.base + DOT_ADDR;

    //3. device initialization
	timer_second = 0;
	counter_count = 0;
	write_to_fnd(0);
	write_to_dot(0);

	//4. request interrupt
	request_interrupt();

    //5. check driver opened
    driver_usage = 1;
	driver_read_usage = 0;
	driver_write_usage = 0;
	driver_ioctl_usage = 0;
	mode_change_toggle = 0;
	timer_start_usage = 0;

	return 0;
}

static int free_interrupt(void)
{
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	printk(KERN_ALERT "Release Module\n");
	return 0;
}

int driver_release(struct inode *minode, struct file *mfile)
{
	//1. free interrupt
	free_interrupt();
	
	//2. device initialization
	timer_second = 0;
	counter_count = 0;
	write_to_fnd(0);
	write_to_dot(10);

	//3. physical memory unmapping
	iounmap(mem.base);
	mem.fnd = NULL;
	mem.dot = NULL;

	//4. reset usage variable
	driver_usage = 0;
	driver_read_usage = 0;
	driver_write_usage = 0;
	driver_ioctl_usage = 0;
	mode_change_toggle = 0;
	timer_start_usage = 0;
	
	printk("%s release\n",driver_NAME);
	return 0;
}
ssize_t driver_read(struct file *inode, char __user *gdata, size_t length, loff_t *off_what)
{
	if(driver_read_usage==0)
	{
		printk("driver read\n");
		driver_read_usage = 1;
		if(copy_to_user(gdata, &counter_count,length))
			return -EFAULT;
		driver_read_usage = 0;
		return length;
	}
	else
	{
		printk("driver_read_usage : %d\n",driver_read_usage);
	}
	return 0;
}
ssize_t driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
	//1. if gdata == 0, sleep thread
	if(gdata == 0)
	{
		if(driver_write_usage==0){
        	    printk("driver main_thread sleep\n");
				driver_write_usage = 1;
                interruptible_sleep_on(&wq_write);
        }
	}
	//2. if not, wake thread
	else
	{
		__wake_up(&wq_write, 1, 1, NULL);
		driver_write_usage = 0;
		del_timer_sync(&timer);
	}
	printk("write\n");
	return 0;
}

int driver_ioctl(struct file* flip, unsigned int cmd, unsigned long arg)
{
	printk ("%s ioctl!\n", driver_NAME);
	
	//1. if mode is counter?
	if(mode_change_toggle)
	{
		printk(KERN_ALERT "mode is in timer\n");
		return IRQ_HANDLED;
	}
	if(driver_ioctl_usage == 0)
	{
		driver_ioctl_usage = 1;
		switch (cmd)
		{
		case DEV_IOCTL_WRITE:
			counter_count = arg;
			printk("counter_count: %lu\n",counter_count);
			write_to_fnd(counter_count);
			write_to_dot(counter_count%10);
			break;
		}
		driver_ioctl_usage = 0;
	}
	else
	{
		printk(KERN_ALERT "driver is in ioctl\n");
		return IRQ_HANDLED;
	}
	return 1;
}

static int driver_major;
static int driver_minor;
static dev_t driver_dev;
static struct cdev driver_cdev;

static int regiseter_driver(void)
{
	int error;

	//1. Setting device number and major number
	if(driver_major)
	{
		//alreay major is set.

		//get device number from major, minor
		driver_dev = MKDEV(driver_major, driver_minor);
		//register character device
		error = register_chrdev_region(driver_dev,1, driver_NAME);
	}
	else
	{
		//major number is not set

		//allocate charcter device
		error = alloc_chrdev_region(&driver_dev,driver_minor,1, driver_NAME);
		//get major number from device number
		driver_major = MAJOR(driver_dev);
	}

	if(error<0) {
		printk(KERN_WARNING "major number setting fail : %d\n", driver_major);
		return -1;
	}
	else
	{
		printk(KERN_ALERT "major number setting success : %d\n", driver_major);
	}

	//2. Register Module
	cdev_init(&driver_cdev, &driver_fops);
	driver_cdev.owner = THIS_MODULE;
	driver_cdev.ops = &driver_fops;
	error = cdev_add(&driver_cdev, driver_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "driver module register fail : %d\n", error);
	}
	return 0;
}
static int unregister_driver(void)
{
	//1. Unregister Module
	cdev_del(&driver_cdev);
	//2. delete device number and major number
	unregister_chrdev_region(driver_dev, 1);
	printk(KERN_ALERT "driver module unregister success\n");
	return 0;
}
int __init driver_init(void)
{
	int result;

	printk("%s init\n", driver_NAME);

    //1. Register driver
	driver_major = driver_MAJOR;
	driver_minor = driver_MINOR;
	result = regiseter_driver();
	if(result <0) {
		printk( "Fail to register : dev_name : %s result : %d\n",
            driver_NAME, result);
		return result;
	}
    printk( "Success to register : dev_file : /dev/%s , major : %d\n", 
        driver_NAME, driver_MAJOR);

	//3. initialize usage variable
    driver_usage = 0;

	//4. initialize timer
	init_timer(&timer);

	printk("Success to initialize\n");
	return 0;
}

void __exit driver_exit(void)
{
	printk("%s exit\n",driver_NAME);

	//1. close driver
	driver_usage = 0;

    //2. unregister driver.
	unregister_driver();
	
	//3. delete timer. (Waiting for handler function by using _sync)
	del_timer_sync(&timer);

    printk("Success to exit\n");
}

module_init( driver_init);
module_exit( driver_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("bn3monkey");