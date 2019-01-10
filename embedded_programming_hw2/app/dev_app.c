#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include <sys/ioctl.h>

#define DEVDRIVER_NAME "/dev/dev_driver"
#define DEVKERNEL_NUM 379

#define DEV_IOCTL_MAGIC 'S'
#define DEV_IOCTL_WRITE _IOW(DEV_IOCTL_MAGIC, 0, int)

int main(int argc, char* argv[])
{
    int gap;
    int times;
    char option[4];

    int param;

    //1. check parameter valid!
    if(argc != 4)
    {
        printf("At least three parameters are required!\n");
        return -1;
    }
    
    //2. set parameter
    gap = (unsigned char)atoi(argv[1]);
    times = (unsigned char)atoi(argv[2]);
    strncpy(option, argv[3], 4);

    //3. system call test
    param = syscall(DEVKERNEL_NUM, gap, times, option);

    //4. driver test
    int fd;
    fd = open(DEVDRIVER_NAME, O_WRONLY);
    if(fd < 0)
    {
        printf("Driver open fail!\n");
        return -1;
    }
    write(fd, &param, sizeof(param));

    ioctl(fd, DEV_IOCTL_WRITE, &param);


    close(fd);



    return 0;
}