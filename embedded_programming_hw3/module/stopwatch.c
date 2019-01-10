#include "stopwatch.h"

static struct file_operations stopwatch_fops =
{
    .open = stopwatch_open, 
    .write = stopwatch_write,
	.release = stopwatch_release,
};
struct timer_list timer, end_timer;

#define BASE_ADDR 0x08000000
#define FND_ADDR 0x4
static struct stopwatch_memory
{
	void __iomem *base;
	void __iomem *fnd;
} mem;

#define fnd_buflen 2
unsigned char fnd[fnd_buflen + 1];

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

//prevent from double open
static int driver_usage;
//prevent from double write
static int drier_write_usage;
//prevent form double start
static int driver_start_usage;

struct stopwatch_param
{
	int minute;
	int second;
} param;
//if value of end_elapsed is 3 second, end the program
static int end_flag;
struct semaphore param_lock;
#define int_to_fnd(v) (((v/10)<<4) + (v)%10)

static void first_tick(struct stopwatch_param* _param)
{
	_param->second = 0;
	_param->minute = 0;
}
static void next_tick(struct stopwatch_param* _param)
{
	_param->second++;
	if(_param->second >= 60)
	{
		_param->second = 0;
		_param->minute++;
	}
	if(_param->minute >= 60)
	{
		_param->minute = 0;
	}
} 
static void stopwatch_tick(unsigned long _param)
{
	
	struct stopwatch_param* now = (struct stopwatch_param *)_param;

	//1. set the next parameter
	down(&param_lock);
	next_tick(now);
	up(&param_lock);

	//2. write to fnd
	printk("NOW TIME (%d : %d)\n",now->minute, now->second);

	fnd[0] = int_to_fnd(now->second);
	fnd[1] = int_to_fnd(now->minute);
	direct_write(mem.fnd, fnd, fnd_buflen);

	//3. add next timer
	timer.expires = get_jiffies_64() + HZ;
	timer.data = _param;
	timer.function = stopwatch_tick;

	add_timer(&timer);
}
static void stopwatch_endtick(unsigned long remained)
{
	// if button end button is pressed and 3 second elapsed, end the program
	if(end_flag)
	{
		printk("END TIME REMAINED : %lu\n", remained);
		if(remained == 0)
		{
			//1. reset fnd
			fnd[0] = 0;
			fnd[1] = 0;
			direct_write(mem.fnd, fnd, fnd_buflen);
			//2. delete ticking timer
			del_timer_sync(&timer);
			//3. wake up main thread
			__wake_up(&wq_write, 1, 1, NULL);
			return;
		}
		//4. go to next tick
		end_timer.expires = get_jiffies_64() + HZ;
		end_timer.data = remained - 1;
		end_timer.function = stopwatch_endtick;

		add_timer(&end_timer);
	}
}

irqreturn_t stopwatch_start(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(START) : %d\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

	//1. Check first start?
	if(driver_start_usage)
		return IRQ_HANDLED; 

	//2. usage setting. (timer is running)
	driver_start_usage = 1;

	//3. write to fnd
	printk("NOW TIME (%d : %d)\n", param.minute, param.second);

	fnd[0] = int_to_fnd(param.second);
	fnd[1] = int_to_fnd(param.minute);
	direct_write(mem.fnd, fnd, fnd_buflen);

	//4. Add first timer
	timer.expires = get_jiffies_64() + HZ;
	timer.data = (unsigned long)(&param);
	timer.function = stopwatch_tick;

	add_timer(&timer);

    return IRQ_HANDLED;
}
irqreturn_t stopwatch_pause(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(PAUSE) : %d\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	//1. Check already start?
	if(!driver_start_usage)
		return IRQ_HANDLED;

	//2. usage setting. (timer is not running)
	driver_start_usage = 0;

	//3. setting end
	del_timer_sync(&timer);

	return IRQ_HANDLED;
}

irqreturn_t stopwatch_reset(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(RESET) : %d\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
	
	//1. reset the timer parameter
	down(&param_lock);
	first_tick(&param);
	up(&param_lock);

	//2. write to fnd
	printk("NOW TIME (%d : %d)\n", param.minute, param.second);

	fnd[0] = int_to_fnd(param.second);
	fnd[1] = int_to_fnd(param.minute);
	direct_write(mem.fnd, fnd, fnd_buflen);

	//3. usage setting. (timer is not running)
	driver_start_usage = 0;

	//4. setting end
	del_timer_sync(&timer);

	return IRQ_HANDLED;
}
irqreturn_t stopwatch_end(int irq, void* dev_id, struct pt_regs* reg)
{
	printk(KERN_ALERT "Interrupt(END) : %d\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	end_flag = 1 - gpio_get_value(IMX_GPIO_NR(5, 14));
	if(end_flag)
	{
		end_timer.expires = get_jiffies_64() + HZ;
		end_timer.data = 3;
		end_timer.function = stopwatch_endtick;
		add_timer(&end_timer);
	}
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
	ret=request_irq(irq, stopwatch_start, IRQF_TRIGGER_FALLING, "home", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, stopwatch_pause, IRQF_TRIGGER_FALLING, "back", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, stopwatch_reset, IRQF_TRIGGER_FALLING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, stopwatch_end, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);

	return 0;
}

int stopwatch_open(struct inode *minode, struct file *mfile)
{
    printk("%s open\n",STOPWATCH_NAME);
    //1. if already open, cancel to open
	if ( driver_usage != 0)
	{
		printk("Fail to open %s : already opened!\n",STOPWATCH_NAME);
		return -EBUSY;
	}

    //2. physical memory mapping
	mem.base = ioremap(BASE_ADDR, 0x1000);
	mem.fnd = mem.base + FND_ADDR;

    //3. fnd initialization
	first_tick(&param);
    fnd[0] = 0;
	fnd[1] = 0;
	direct_write(mem.fnd, fnd, fnd_buflen);

	//4. request interrupt
	request_interrupt();

	//5. semaphore initialzation
	sema_init(&param_lock, 1);

    //6. check driver opened
    driver_usage = 1;
	drier_write_usage = 0;
	driver_start_usage = 0;
	end_flag = 0;
	
	return 0;
}

static int free_interrupt(void)
{
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	printk(KERN_ALERT "Release Module\n");
	return 0;
}

int stopwatch_release(struct inode *minode, struct file *mfile)
{
	//1. free interrupt
	free_interrupt();
	
	//2. physical memory unmapping
	iounmap(mem.base);
	mem.fnd = NULL;
	
	//3. reset usage variable
	driver_usage = 0;
	drier_write_usage = 0;
	driver_start_usage = 0;
	end_flag = 0;

	printk("%s release\n",STOPWATCH_NAME);
	return 0;
}
ssize_t stopwatch_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
	//1. sleep main thread
	if(drier_write_usage==0){
                printk("stopwatch main_thread sleep\n");
				drier_write_usage += 1;
                interruptible_sleep_on(&wq_write);
        }

	printk("write\n");
	return 0;
}

static int stopwatch_major;
static int stopwatch_minor;
static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;

static int regiseter_stopwatch(void)
{
	int error;

	//1. Setting device number and major number
	if(stopwatch_major)
	{
		//alreay major is set.

		//get device number from major, minor
		stopwatch_dev = MKDEV(stopwatch_major, stopwatch_minor);
		//register character device
		error = register_chrdev_region(stopwatch_dev,1, STOPWATCH_NAME);
	}
	else
	{
		//major number is not set

		//allocate charcter device
		error = alloc_chrdev_region(&stopwatch_dev,stopwatch_minor,1, STOPWATCH_NAME);
		//get major number from device number
		stopwatch_major = MAJOR(stopwatch_dev);
	}

	if(error<0) {
		printk(KERN_WARNING "major number setting fail : %d\n", stopwatch_major);
		return -1;
	}
	else
	{
		printk(KERN_ALERT "major number setting success : %d\n", stopwatch_major);
	}

	//2. Register Module
	cdev_init(&stopwatch_cdev, &stopwatch_fops);
	stopwatch_cdev.owner = THIS_MODULE;
	stopwatch_cdev.ops = &stopwatch_fops;
	error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "stopwatch module register fail : %d\n", error);
	}
	return 0;
}
static int unregister_stopwatch(void)
{
	//1. Unregister Module
	cdev_del(&stopwatch_cdev);
	//2. delete device number and major number
	unregister_chrdev_region(stopwatch_dev, 1);
	printk(KERN_ALERT "stopwatch module unregister success\n");
	return 0;
}
int __init stopwatch_init(void)
{
	int result;

	printk("%s init\n", STOPWATCH_NAME);

    //1. Register driver
	stopwatch_major = STOPWATCH_MAJOR;
	stopwatch_minor = STOPWATCH_MINOR;
	result = regiseter_stopwatch();
	if(result <0) {
		printk( "Fail to register : dev_name : %s result : %d\n",
            STOPWATCH_NAME, result);
		return result;
	}
    printk( "Success to register : dev_file : /dev/%s , major : %d\n", 
        STOPWATCH_NAME, STOPWATCH_MAJOR);

	//2. initialize timer
	init_timer(&timer);
	init_timer(&end_timer);

    //3. initialize usage variable
    driver_usage = 0;

	printk("Success to initialize\n");
	return 0;
}

void __exit stopwatch_exit(void)
{
	printk("%s exit\n",STOPWATCH_NAME);

	//1. close driver
	driver_usage = 0;

    //2. unregister driver.
	unregister_stopwatch();
	
	//3. delete timer. (Waiting for handler function by using _sync)
	del_timer_sync(&timer);
	del_timer_sync(&end_timer);

    printk("Success to exit\n");
}

module_init( stopwatch_init);
module_exit( stopwatch_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("bn3monkey");