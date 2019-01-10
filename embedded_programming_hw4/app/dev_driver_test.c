#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include <sys/ioctl.h>
#define DEV_IOCTL_MAGIC 'S'
#define DEV_IOCTL_WRITE _IOW(DEV_IOCTL_MAGIC, 0, int)

#define DEVDRIVER_NAME "/dev/dev_driver"

int main(/*int argc, char* argv[]*/)
{

    int fd;
    int retn;

    int trial = 5;
    int send = 5;
    int receive = 5;

    fd = open(DEVDRIVER_NAME, O_RDWR);
    if(fd < 0)
    {
        printf("Driver open fail!\n");
        return -1;
    }
    int pid;
    pid = fork();
    if(pid == 0)
    {
        while(trial--)
        {
            getchar();
            ioctl(fd, DEV_IOCTL_WRITE, send);

            retn = read(fd, &receive, sizeof(int));
            printf("receive : %d\n", receive);
            send += receive;
        }
    }
    else
    {
        retn = write(fd, NULL, 0);
        close(fd);
    }
}