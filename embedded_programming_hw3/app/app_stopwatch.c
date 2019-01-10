#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include <sys/ioctl.h>

#define DEVDRIVER_NAME "/dev/stopwatch"

int main(/*int argc, char* argv[]*/)
{

    int fd;
    int retn;

    fd = open(DEVDRIVER_NAME, O_WRONLY);
    if(fd < 0)
    {
        printf("Driver open fail!\n");
        return -1;
    }

    retn = write(fd, NULL, 0);
    close(fd);
}