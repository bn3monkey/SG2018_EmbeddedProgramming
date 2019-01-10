#include "output_proc.h"
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
//For 'mmap'-----
#include <sys/mman.h>
//----------------------

/************** Device Setting ************/
static char* output_device_file[OUTPUT_DEVICE_NUM] = 
{
    "/dev/mem",
    "/dev/fpga_fnd",
    "/dev/fpga_text_lcd",
    "/dev/fpga_dot"
};
int output_device_fd[OUTPUT_DEVICE_NUM];

//LED physical address
unsigned char *led_addr;

static int openOutputDevice()
{
    unsigned long *fpga_addr = 0;

    //LED OPEN & MMAP
    output_device_fd[OUTPUT_DEVICE_LED] = open(output_device_file[OUTPUT_DEVICE_LED], O_RDWR | O_SYNC);
    if(output_device_fd[OUTPUT_DEVICE_LED] == -1)
    {
        fprintf (stderr, "ERROR : %s Device Open fail\n", output_device_file[OUTPUT_DEVICE_LED]);
        return FALSE;
    }
    fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, output_device_fd[OUTPUT_DEVICE_LED], FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		fprintf (stderr, "ERROR : %s Device MMAP fail\n", output_device_file[OUTPUT_DEVICE_LED]);
		close(output_device_fd[OUTPUT_DEVICE_LED]);
		return FALSE;
	}
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
    #ifdef DEBUG_OUTPUT
    fprintf(stdout, "Open Output %d Device\n",OUTPUT_DEVICE_LED);
    #endif


    //FND OPEN
    output_device_fd[OUTPUT_DEVICE_FND] = open(output_device_file[OUTPUT_DEVICE_FND], O_RDWR);
    if(output_device_fd[OUTPUT_DEVICE_FND] < 0)
    {
        fprintf (stderr, "ERROR : %s Device Open fail\n", output_device_file[OUTPUT_DEVICE_FND]);
        return FALSE;
    }
    #ifdef DEBUG_OUTPUT
    fprintf(stdout, "Open Output %d Device\n",OUTPUT_DEVICE_FND);
    #endif
    
    //TEXT OPEN
    output_device_fd[OUTPUT_DEVICE_TEXT] = open(output_device_file[OUTPUT_DEVICE_TEXT], O_WRONLY);
    if(output_device_fd[OUTPUT_DEVICE_TEXT] < 0)
    {
        fprintf (stderr, "ERROR : %s Device Open fail\n", output_device_file[OUTPUT_DEVICE_TEXT]);
        return FALSE;
    }
    #ifdef DEBUG_OUTPUT
    fprintf(stdout, "Open Output %d Device\n",OUTPUT_DEVICE_TEXT);
    #endif

    //DOT OPEN
    output_device_fd[OUTPUT_DEVICE_DOT] = open(output_device_file[OUTPUT_DEVICE_DOT], O_WRONLY);
    if(output_device_fd[OUTPUT_DEVICE_DOT] < 0)
    {
        fprintf (stderr, "ERROR : %s Device Open fail\n", output_device_file[OUTPUT_DEVICE_DOT]);
        return FALSE;
    }
    #ifdef DEBUG_OUTPUT
    fprintf(stdout, "Open Output %d Device\n",OUTPUT_DEVICE_DOT);
    #endif

    #ifdef DEBUG_OUTPUT
    fprintf(stdout, "Open Output Device!\n");
    #endif
    return TRUE;
}
static int closeOutputDevice()
{
    int success = TRUE;
    if(close(output_device_fd[OUTPUT_DEVICE_LED]) == -1)
    {
        fprintf (stderr, "ERROR : %s Device Close fail\n", output_device_file[OUTPUT_DEVICE_LED]);
        success =  FALSE;
    }
    if(close(output_device_fd[OUTPUT_DEVICE_FND]) == -1)
    {
        fprintf (stderr, "ERROR : %s Device Close fail\n", output_device_file[OUTPUT_DEVICE_FND]);
        success =  FALSE;
    }
    if(close(output_device_fd[OUTPUT_DEVICE_TEXT]) == -1)
    {
        fprintf (stderr, "ERROR : %s Device Close fail\n", output_device_file[OUTPUT_DEVICE_TEXT]);
        success =  FALSE;
    }
    if(close(output_device_fd[OUTPUT_DEVICE_DOT]) == -1)
    {
        fprintf (stderr, "ERROR : %s Device Close fail\n", output_device_file[OUTPUT_DEVICE_DOT]);
        success =  FALSE;
    }

    #ifdef DEBUG_OUTPUT
    fprintf(stdout, "Close Output Device!\n");
    #endif
    return success;
}
/******************************************/

/********** Device Output function *********/
void (*output_device_table[OUTPUT_DEVICE_NUM]) (char* local);
void output_func_LED(char* local)
{
    char* msg = local + output_index[OUTPUT_DEVICE_LED];
    char flag = msg[0];
    if(flag == INVALID) return;

    msg++;
    *led_addr = *msg;
}
void output_func_FND(char* local)
{
    int err;
    unsigned char* msg = (unsigned char *)local + output_index[OUTPUT_DEVICE_FND];
    char flag = msg[0];
    if(flag == INVALID) return;

    msg[0] = INVALID;
    set_output((const char *)msg, OUTPUT_DEVICE_FND);
    
    unsigned char target[4] = {0};
    memcpy(target, msg+1, 4);

    err = write(output_device_fd[OUTPUT_DEVICE_FND], &target, 
        output_size[OUTPUT_DEVICE_FND] -1);	
    if(err < 0) 
        fprintf(stderr, "ERROR : FND Write Error!\n");

}
void output_func_TEXT(char* local)
{
    int err;
    char* msg = local + output_index[OUTPUT_DEVICE_TEXT];
    char flag = msg[0];
    if(flag == INVALID) return;

    msg[0] = INVALID;
    set_output((const char *)msg, OUTPUT_DEVICE_TEXT);
    
    err = write(output_device_fd[OUTPUT_DEVICE_TEXT], msg+1, 
        output_size[OUTPUT_DEVICE_TEXT] -1);
    if(err < 0)
       fprintf(stderr, "ERROR : TEXT Write Error!\n");
}
void output_func_DOT(char* local)
{
    int err;
    char* msg = local + output_index[OUTPUT_DEVICE_DOT];
    char flag = msg[0];
    if(flag == INVALID) return;

    msg[0] = INVALID;
    set_output((const char *)msg, OUTPUT_DEVICE_DOT);

    err = write(output_device_fd[OUTPUT_DEVICE_DOT], msg+1, 
        output_size[OUTPUT_DEVICE_DOT] -1);
    if(err < 0)
       fprintf(stderr, "ERROR : DOT Write Error!\n");
}
/*******************************************/

/*********** Mode Output function **********/
void outputRESET()
{
    int err;
    unsigned char resetter[33] = {0};

    //LED RESET
    *led_addr = 0;

    //FND RESET
    err = write(output_device_fd[OUTPUT_DEVICE_FND], &resetter, 
        output_size[OUTPUT_DEVICE_FND] -1);	
    if(err < 0) 
        fprintf(stderr, "ERROR : FND Write Error!\n");
        
    //TEXT RESET
    err = write(output_device_fd[OUTPUT_DEVICE_TEXT], resetter, 
        output_size[OUTPUT_DEVICE_TEXT] -1);
    if(err < 0)
       fprintf(stderr, "ERROR : TEXT Write Error!\n");

    //DOT RESET
    err = write(output_device_fd[OUTPUT_DEVICE_DOT], resetter, 
        output_size[OUTPUT_DEVICE_DOT] -1);
    if(err < 0)
       fprintf(stderr, "ERROR : DOT Write Error!\n");
    
    set_output((const char *)resetter, OUTPUT_DEVICE_LED);

    fprintf(stdout, "OUTPUT_DEVICE_ALL_RESET!\n");

    unlock_sema(reset_message_sema_id);
}
void outputCLOCK(char* local)
{
    output_func_FND(local);
    output_func_LED(local);
}
void outputCOUNTER(char* local)
{
    output_func_FND(local);
    output_func_LED(local);
}
void outputTEXTEDITOR(char* local)
{
     output_func_FND(local);
     output_func_TEXT(local);
     output_func_DOT(local);
}
void outputDRAWBOARD(char* local)
{
    output_func_FND(local);
    output_func_DOT(local);
}
void outputEXTRA(char* local)
{
    output_func_LED(local);
    output_func_FND(local);
    output_func_TEXT(local);
    output_func_DOT(local);
}
void (*output_func_table[MODE_NUM]) (char* local) = 
{
     outputCLOCK
    ,outputCOUNTER
    ,outputTEXTEDITOR
    ,outputDRAWBOARD
    ,outputEXTRA
};
void outputMODE()
{
    char local[OUTPUT_MESSAGE_ELEM_SIZE+1];
    get_output(local);

    if(local[0] == RESET)
    {
        outputRESET();
    }
    else
        output_func_table[*vaddr_current_mode] (local);
}
/*******************************************/

/**** initialization & Destroy function *****/
//get shared memory for input process
static int getOutputSharedMemory()
{
    if(!get_shared_memory(CURRENT_MODE))
    {
        fprintf(stderr, "ERROR : output process fail to get CURRENT_MODE!\n");
        return FALSE;
    }

    if(!get_shared_memory(OUTPUT_MESSAGE))
    {
        fprintf(stderr, "ERROR : output process fail to get OUTPUT_MESSAGE!\n");
        return FALSE;
    }
    if(!get_shared_memory(ALLKILL_TRIGGER))
    {
        fprintf(stderr, "ERROR : output process fail to get ALLKILL_TRIGGER!\n");
        return FALSE;
    }
    #ifdef DEBUG_OUTPUT
        fprintf(stdout, "Get output shared Memory!\n");
    #endif
    return TRUE;
}
static int getOutputSemaphore()
{
    if(!get_semaphore(RESET_MASSAGE_SEMA))
    {
        fprintf(stderr, "ERROR : output process fail to get RESET_MASSAGE_SEMA!\n");
        return FALSE;       
    }
    if(!get_semaphore(OUTPUT_MESSAGE_SEMA))
    {
        fprintf(stderr, "ERROR : output process fail to get OUTPUT_MESSAGE_SEMA!\n");
        return FALSE;
    }
    #ifdef DEBUG_OUTPUT
        fprintf(stdout, "Get output Semaphore!\n");
    #endif
    return TRUE;
}

int output_init()
{
    // 1) shared_memory get
    if(!getOutputSharedMemory())
        return FALSE;
    // 2) semaphore get
    if(!getOutputSemaphore())
        return FALSE;
    // 3) Device Open
    if(!openOutputDevice())
        return FALSE;

    return TRUE;
}

int output_deinit()
{
    outputRESET();
    if(!closeOutputDevice())
        return FALSE;
    return TRUE;
}

/******************************************/

void output_proc()
{
    if(!output_init())
    {
        setKillTrigger();
        fprintf(stderr, "ERROR : Output Process Error!\n");
        goto END_INPUTPROC;
    }

    //if Trigger is On, End below iteration and exit the function.
    while(!getKillTrigger())
    {
        outputMODE();
    }

    END_INPUTPROC:
    output_deinit();
}