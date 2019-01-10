#include <linux/kernel.h>
#include <linux/uaccess.h>

struct sys_mydev_parameter
{
    unsigned char fnd_index;
	unsigned char fnd_value;
	unsigned char gap;
	unsigned char times;
};

asmlinkage int sys_getdevparam(int gap, int times, unsigned char* option) {
	struct sys_mydev_parameter result;
    unsigned char koption[4];
    unsigned char i;
    int success;

	success = copy_from_user(koption, option, 4*sizeof(unsigned char));
    if(success)
    {
        printk("ERROR : copy_from_user fail\n");
        return -1;
    }

	if(gap <= 0 || gap > 100)
    {
        printk("ERROR : gap is not in 0 to 100\n");
        return -1;
    }
    result.gap = (unsigned char)gap;

    if(times <=0 || times > 100)
    {
        printk("ERROR : times is not in 0 to 100\n");
        return -1;
    }
    result.times = (unsigned char)times;

    for(i=0;i<4;i++)
    {
        if(option[i] < '0' && option[i] > '9')
        {
            printk("ERROR : option is not in '0' to '9'\n");
            return -1;
        }
        if(option[i] != '0')
        {
            result.fnd_index = i;
            result.fnd_value = option[i]-'0';
        }
    }
    printk("--SYSTEM CALL GET--\ngap(%d) times(%d) fnd_index(%d) fnd_value(%d)\n",
        result.gap, result.times, result.fnd_index, result.fnd_value);

    printk("int transform : %d\n",*(int *)(&result));

	return *(int *)(&result);
}