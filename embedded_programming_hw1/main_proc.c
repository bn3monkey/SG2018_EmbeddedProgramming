#include "main_proc.h"
#include "shared_proc.h"


pid_t pid_input;
pid_t pid_output;

int main(/*int argc, char** argv*/)
{
    int proc_name = CAL_PROC;
    int wait_status;

    //1. make shared_memory
    make_shared_memory();

    //2. make semaphore
    make_semaphore();

    //3. make child process (input/output) 
    pid_input = fork();
    if(pid_input < 0)
    {
        //fork fail
        fprintf(stderr, "ERROR : input process fork fail!\n");
        return 0;
    }
    else if(pid_input == 0)
    {
        //in input process
        proc_name = INPUT_PROC;
    }
    else
    {
        //in cal process
        pid_output = fork();
        if(pid_output < 0)
        {
            //fork fail!
            wait(&wait_status);
            fprintf(stderr, "ERROR : input process fork fail!\n");
            return 0;
        }
        else if(pid_output == 0)
        {
            //in output process
            proc_name = OUTPUT_PROC;
        }
    }

    switch(proc_name)
    {
        case INPUT_PROC : 
            printf(" Only one input process!\n");
            input_proc(); 
            
            break;
        case CAL_PROC : 
            printf(" Only one cal process!\n");
            cal_proc(); 

            wait(&wait_status);
            delete_shared_memory();
            delete_semaphore();   
            break;

        case OUTPUT_PROC :
            printf(" Only one output process!\n"); 
            output_proc(); 
            
            break;
    }

    
    return 0;
}