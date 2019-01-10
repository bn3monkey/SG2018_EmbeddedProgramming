#include "input_proc.h"
#include "shared_proc.h"


#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//For function 'open'-- 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//----------------------
//For function 'read,'close'--
#include <unistd.h>
//----------------------
//For 'input Event'-----
#include <linux/input.h>
//----------------------

/************** Device Setting ************/
static char* input_device_file[INPUT_DEVICE_NUM] = 
{
    "/dev/input/event0",
    "/dev/fpga_push_switch",
};
int input_device_fd[INPUT_DEVICE_NUM];

static int openInputDevice()
{
    input_device_fd[INPUT_DEVICE_SIDE] = open(input_device_file[INPUT_DEVICE_SIDE], O_RDONLY|O_NONBLOCK);
    if(input_device_fd[INPUT_DEVICE_SIDE] == -1)
    {
        fprintf (stderr, "ERROR : %s Device Open fail\n", input_device_file[INPUT_DEVICE_SIDE]);
        return FALSE;
    }
    #ifdef DEBUG_INPUT
    fprintf(stdout, "Open Input %d Device\n",INPUT_DEVICE_SIDE);
    #endif
    
    input_device_fd[INPUT_DEVICE_NUMPAD] = open(input_device_file[INPUT_DEVICE_NUMPAD], O_RDWR);
    if(input_device_fd[INPUT_DEVICE_NUMPAD] < 0)
    {
        fprintf (stderr, "ERROR : %s Device Open fail\n", input_device_file[INPUT_DEVICE_NUMPAD]);
        return FALSE;
    }
    #ifdef DEBUG_INPUT
    fprintf(stdout, "Open Input %d Device\n",INPUT_DEVICE_NUMPAD);
    #endif
    
    #ifdef DEBUG_INPUT
    fprintf(stdout, "Open Input Device!\n");
    #endif
    return TRUE;
}
static int closeInputDevice()
{
    int success = TRUE;
    if(close(input_device_fd[INPUT_DEVICE_SIDE]) == -1)
    {
        fprintf (stderr, "ERROR : %s Device Close fail\n", input_device_file[INPUT_DEVICE_SIDE]);
        success =  FALSE;
    }
    if(close(input_device_fd[INPUT_DEVICE_NUMPAD]) == -1)
    {
        fprintf (stderr, "ERROR : %s Device Close fail\n", input_device_file[INPUT_DEVICE_SIDE]);
        success =  FALSE;
    }

    #ifdef DEBUG_INPUT
    fprintf(stdout, "Close Input Device!\n");
    #endif
    return success;
}
/******************************************/

/*********** Mode input function **********/
void input_side()
{
    int size = sizeof (struct input_event);
    struct input_event ev[BUFF_SIZE];
    int err_snd;

    queue_elem elem;
    elem.mtype = INPUT_MODE_QUEUE;

    if (read(input_device_fd[INPUT_DEVICE_SIDE], ev, size * BUFF_SIZE) >= size)
    {
        if (ev[0].value == KEY_PRESSED)
        {
            switch(ev[0].code)
            {
                case 114 : elem.data[0] = SIDE_PREVMODE; break;
                case 115 : elem.data[0] = SIDE_NEXTMODE; break;
                case 158 : elem.data[0] = SIDE_EXIT; break;
            }
            err_snd = msgsnd(input_mode_queue_id, (const void *)(&elem), QUEUE_ELEM_SIZE, IPC_NOWAIT);
            
            #ifdef DEBUG_INPUT
            if (err_snd == -1)
                fprintf(stdout, "Cannot Send Input Side Message\n");
            else
                fprintf(stdout, "Success to Send!\n");
            #endif
        }
    }
    
}
/******************************************/

/******** specifies input function ********/
static void inputNumpad()
{
    
    int size = sizeof(char)*MAX_BUTTON;
    int err_snd, i;

    static unsigned char prev[MAX_BUTTON];;
    unsigned char now[MAX_BUTTON] = {0};
    
    queue_elem elem;
    elem.mtype = INPUT_SWITCH_QUEUE;
    int* data = (int *)(elem.data);
    *data = 0;

    usleep(200000);

    i = read(input_device_fd[INPUT_DEVICE_NUMPAD], &now, size);
    if(i < 0)
        fprintf(stderr, "ERROR : read error!\n");

    
    for(i=0;i<MAX_BUTTON;i++)
    {
        //printf("prev[%d] = %d, now[%d] = %d\n",i,prev[i],i,now[i]);
        if(prev[i] == KEY_RELEASE && now[i] == KEY_PRESSED)
        {
            *data |= (1 << i);
        }
    }

    memcpy(prev, now, sizeof(unsigned char) * MAX_BUTTON);

    if(*data != 0)
    {
        err_snd = msgsnd(input_switch_queue_id, (const void *)(&elem), QUEUE_ELEM_SIZE, IPC_NOWAIT);
            
        #ifdef DEBUG_INPUT
        if (err_snd == -1)
            fprintf(stdout, "Cannot Send Input Switch Message\n");
        else
            fprintf(stdout, "Success to Send!\n");
        #endif
    }
}
static void inputExtra()
{
    int size = sizeof(char)*MAX_BUTTON;
    int err_snd, i;

    static unsigned char prev[MAX_BUTTON];;
    unsigned char now[MAX_BUTTON] = {0};
    
    queue_elem elem;
    elem.mtype = INPUT_SWITCH_QUEUE;
    int* data = (int *)(elem.data);
    *data = 0;

    usleep(10000);

    i = read(input_device_fd[INPUT_DEVICE_NUMPAD], &now, size);
    if(i < 0)
        fprintf(stderr, "ERROR : read error!\n");

    
    for(i=0;i<MAX_BUTTON;i++)
    {
        //printf("prev[%d] = %d, now[%d] = %d\n",i,prev[i],i,now[i]);
        if(prev[i] == KEY_RELEASE && now[i] == KEY_PRESSED)
        {
            *data |= (1 << i);
        }
    }

    memcpy(prev, now, sizeof(unsigned char) * MAX_BUTTON);

    if(*data != 0)
    {
        err_snd = msgsnd(input_switch_queue_id, (const void *)(&elem), QUEUE_ELEM_SIZE, IPC_NOWAIT);
            
        #ifdef DEBUG_INPUT
        if (err_snd == -1)
            fprintf(stdout, "Cannot Send Input Switch Message\n");
        else
            fprintf(stdout, "Success to Send!\n");
        #endif
    }
}
void (*input_func_table[MODE_NUM]) () = 
{
    inputNumpad,
    inputNumpad,
    inputNumpad,
    inputNumpad,
    inputExtra,
};
void input_pad()
{
    input_func_table[*vaddr_current_mode] ();
}
/******************************************/

/**** initialization & Destroy function *****/
//get shared memory for input process
static int getInputSharedMemory()
{
    if(!get_shared_memory(CURRENT_MODE))
    {
        fprintf(stderr, "ERROR : input process fail to get CURRENT_MODE!\n");
        return FALSE;
    }
    if(!get_shared_memory(INPUT_MODE_QUEUE))
    {
        fprintf(stderr, "ERROR : input process fail to get INPUT_MODE_QUEUE!\n");
        return FALSE;
    }
    if(!get_shared_memory(INPUT_SWITCH_QUEUE))
    {
        fprintf(stderr, "ERROR : input process fail to get INPUT_SWITCH_QUEUE!\n");
        return FALSE;
    }
    if(!get_shared_memory(ALLKILL_TRIGGER))
    {
        fprintf(stderr, "ERROR : input process fail to get ALLKILL_TRIGGER!\n");
        return FALSE;
    }
    #ifdef DEBUG_INPUT
        fprintf(stdout, "Get input shared Memory!\n");
    #endif
    return TRUE;
}
static int getInputSemaphore()
{
    //Semaphore doesn't needed
    #ifdef DEBUG_INPUT
        fprintf(stdout, "Get input Semaphore!\n");
    #endif
    return TRUE;
}

int input_init()
{
    // 1) shared_memory get
    if(!getInputSharedMemory())
        return FALSE;
    // 2) semaphore get
    if(!getInputSemaphore())
        return FALSE;
    // 3) Device Open
    if(!openInputDevice())
        return FALSE;

    return TRUE;
}

int input_deinit()
{
    if(!closeInputDevice())
        return FALSE;
    return TRUE;
}

/******************************************/

void input_proc()
{
    if(!input_init())
    {
        setKillTrigger();
        fprintf(stderr, "ERROR : Input Process Error!\n");
        goto END_INPUTPROC;
    }

    //if Trigger is On, End below iteration and exit the function.
    while(!getKillTrigger())
    {
        input_side();
        input_pad();
    }

    END_INPUTPROC:
    input_deinit();
}