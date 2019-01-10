#include "shared_proc.h"


/** Shared Memory Section **/
int current_mode_id;
int input_mode_queue_id;
int input_switch_queue_id;
int output_message_id;
int allkill_trigger_id;

int* vaddr_current_mode;
char* vaddr_output_message;
int* vaddr_allkill_trigger;

int make_shared_memory()
{
    current_mode_id = shmget(CURRENT_MODE, CURRENT_MODE_ELEM_SIZE, IPC_CREAT|0664);
    if(current_mode_id == -1)
    {
        fprintf(stderr, "ERROR : current_mode allocation error!\n");
        return FALSE;
    }
    //First Mode is Clock
    vaddr_current_mode = shmat(current_mode_id, (int *)NULL, 0);
    *vaddr_current_mode = MODE_CLOCK;

    input_mode_queue_id = msgget(INPUT_MODE_QUEUE, IPC_CREAT|0666);
    if(input_mode_queue_id == -1)
    {
        fprintf(stderr, "ERROR : input_mode_queue allocation error!\n");
        return FALSE;
    }

    input_switch_queue_id = msgget(INPUT_SWITCH_QUEUE, IPC_CREAT|0666);
    if(input_switch_queue_id == -1)
    {
        fprintf(stderr, "ERROR : input_switch_queue allocation error!\n");
        return FALSE;
    }

    output_message_id = shmget(OUTPUT_MESSAGE, OUTPUT_MESSAGE_ELEM_SIZE, IPC_CREAT|0666);
    if(output_message_id == -1)
    {
        fprintf(stderr, "ERROR : output_message allocation error!\n");
        return FALSE;
    }
    vaddr_output_message = shmat(output_message_id, (char *)NULL, 0);
    memset(vaddr_output_message, 0, OUTPUT_MESSAGE_ELEM_SIZE);

    allkill_trigger_id = shmget(ALLKILL_TRIGGER, ALLKILL_TRIGGER_ELEM_SIZE, IPC_CREAT|0666);
    if(allkill_trigger_id == -1)
    {
        fprintf(stderr, "ERROR : allkill_trigger allocation error!\n");
        return FALSE;
    }
    //First, allkill trigger initialize by zero. it means no process have to die yet.
    vaddr_allkill_trigger = shmat(allkill_trigger_id, (int *)NULL, 0);
    *vaddr_allkill_trigger = FALSE;
    
    #ifdef DEBUG_SHARED
        fprintf(stdout, "Make Shared Memory!\n");
    #endif

    return TRUE;
}
int get_shared_memory(int type)
{
    switch(type)
    {
        case CURRENT_MODE:
            current_mode_id = shmget(CURRENT_MODE, CURRENT_MODE_ELEM_SIZE, 0666);
            if(current_mode_id == -1)
            {
                fprintf(stderr, "ERROR : current_mode not found!\n");
                return FALSE;
            }
            vaddr_current_mode = shmat(current_mode_id, (int *)NULL, 0); 
            break;
        case INPUT_MODE_QUEUE:
            input_mode_queue_id = msgget(INPUT_MODE_QUEUE, 0666);
            if(input_mode_queue_id == -1)
            {
                fprintf(stderr, "ERROR : input_mode_queue not found!\n");
                return FALSE;
            }
            break;
        case INPUT_SWITCH_QUEUE:
            input_switch_queue_id = msgget(INPUT_SWITCH_QUEUE, 0666);
            if(input_switch_queue_id == -1)
            {
                fprintf(stderr, "ERROR : input_switch_queue not found!\n");
                return FALSE;
            } 
            break;
        case OUTPUT_MESSAGE:
            output_message_id = shmget(OUTPUT_MESSAGE, OUTPUT_MESSAGE_ELEM_SIZE, 0666);
            if(output_message_id == -1)
            {
                fprintf(stderr, "ERROR : output_message_queue not found!\n");
                return FALSE;
            }
            vaddr_output_message = shmat(output_message_id, (char *)NULL, 0); 
            break;
        case ALLKILL_TRIGGER:
            allkill_trigger_id = shmget(ALLKILL_TRIGGER, ALLKILL_TRIGGER_ELEM_SIZE, 0666);
            if(allkill_trigger_id == -1)
            {
                fprintf(stderr, "ERROR : allkill_trigger not found!\n");
                return FALSE;
            }
            vaddr_allkill_trigger = shmat(allkill_trigger_id, (int *)NULL, 0); 
            break;
    }

    #ifdef DEBUG_SHARED
        fprintf(stdout, "GET SHARED MEMORY : %d!\n", type);
    #endif
    return TRUE;
}
int delete_shared_memory()
{
    int success = TRUE;
    if(shmdt(vaddr_current_mode)==-1)
    {
        fprintf(stderr, "ERROR : current_mode detach error!\n");
        success = FALSE;
    }
    if(shmctl(current_mode_id, IPC_RMID, (struct shmid_ds *)NULL)==-1)
    {
        fprintf(stderr, "ERROR : current_mode delete error!\n");
        success = FALSE;    
    }

    if(msgctl(input_mode_queue_id, IPC_RMID, (struct msqid_ds *)NULL)==-1)
    {
        fprintf(stderr, "ERROR : input_mode_queue delete error!\n");
        success = FALSE;
    }

 
    if(msgctl(input_switch_queue_id, IPC_RMID, (struct msqid_ds *)NULL)==-1)
    {
        fprintf(stderr, "ERROR : input_switch_queue delete error!\n");
        success = FALSE;
    }

    if(shmdt(vaddr_output_message)==-1)
    {
        fprintf(stderr, "ERROR : output_message detach error!\n");
        success = FALSE;
    }
    if(shmctl(output_message_id, IPC_RMID, (struct shmid_ds *)NULL)==-1)
    {
        fprintf(stderr, "ERROR : output_message delete error!\n");
        success = FALSE;
    }

    if(shmdt(vaddr_allkill_trigger)==-1)
    {
        fprintf(stderr, "ERROR : allkill_trigger detach error!\n");
        success = FALSE;
    }
    if(shmctl(allkill_trigger_id, IPC_RMID, (struct shmid_ds *)NULL)==-1)
    {
        fprintf(stderr, "ERROR : allkill_trigger delete error!\n");
        success = FALSE;    
    }

    #ifdef DEBUG_SHARED
        fprintf(stdout, "Delete Shared Memory!\n");
    #endif
    return success;
}

int getKillTrigger()
{
    int trigger;
    trigger = *vaddr_allkill_trigger;
    #ifdef DEBUG_KILLTRIGGER
        if(trigger == 1) printf("all_kill trigger On!\n");
    #endif
    return trigger;
}
void setKillTrigger()
{
    //trigger changes only 0 to 1 or 1 to 1.
    //and all process check that trigger is 0 for kill process
    //semaphore doesn't need. 
    *vaddr_allkill_trigger = 1;
    #ifdef DEBUG_KILLTRIGGER
        printf("all_kill trigger setted by 1!\n");
    #endif
    return;
}

/***************** semaphore section *******************/
int reset_message_sema_id;
int output_message_sema_id;


int make_semaphore()
{
    union semun sema_union;
    
    sema_union.val = 0;
    reset_message_sema_id = semget(RESET_MASSAGE_SEMA, 1, IPC_CREAT|0664);
    if(reset_message_sema_id == -1)
    {
        fprintf(stderr, "ERROR : reset_message_sema allocation error!\n");
        return FALSE;
    }
    
    if (semctl(reset_message_sema_id, 0, SETVAL, sema_union)==-1)
    {   
        fprintf(stderr, "ERROR : reset_message_sema setting error!\n");
        return FALSE; 
    }

    sema_union.val = 1;
    output_message_sema_id = semget(OUTPUT_MESSAGE_SEMA, 1, IPC_CREAT|0664);
    if(output_message_sema_id == -1)
    {
        fprintf(stderr, "ERROR : output_message_sema allocation error!\n");
        return FALSE;
    }
    if (semctl( output_message_sema_id, 0, SETVAL, sema_union)==-1)
    {   
        fprintf(stderr, "ERROR : output_message_sema setting error!\n");
        return FALSE; 
    }

    #ifdef DEBUG_SHARED
        fprintf(stdout, "Make Semaphore!\n");
    #endif

    return TRUE;
}
int get_semaphore(int type)
{
    switch(type)
    {
        case RESET_MASSAGE_SEMA :
        reset_message_sema_id = semget(RESET_MASSAGE_SEMA, 1, 0664);
        if (reset_message_sema_id == -1)
        {
            fprintf(stderr, "ERROR : reset_message_sema allocation error!\n");
            return FALSE;
        }
        break;
        case OUTPUT_MESSAGE_SEMA:
        output_message_sema_id = semget(OUTPUT_MESSAGE_SEMA, 1, 0664);
        if (output_message_sema_id == -1)
        {
            fprintf(stderr, "ERROR : output_message_sema allocation error!\n");
            return FALSE;
        }
        break;
    }

    #ifdef DEBUG_SHARED
        fprintf(stdout, "Get Semaphore : %d!\n",type);
    #endif

    return TRUE;
}
int delete_semaphore()
{
    int success = TRUE;
    if(semctl(reset_message_sema_id, 0, IPC_RMID) == -1)
    {
        fprintf(stderr, "ERROR : reset_message_sema delete error!\n");
        success = FALSE;
    }
    if(semctl(output_message_sema_id, 0, IPC_RMID) == -1)
    {
        fprintf(stderr, "ERROR : output_message_sema delete error!\n");
        success = FALSE;
    }

    #ifdef DEBUG_SHARED
        fprintf(stdout, "Delete Semaphore!\n");
    #endif

    return success;
}

int lock_sema(int id)
{
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;

    if(semop(id, &buf, 1) == -1)
    {
        fprintf(stderr, "ERROR : Semaphore lock fail");
        //announce kill message to other proccess
        setKillTrigger();
        //kill this proccess immediately for prevent confliction of shared memory
        exit(-1);
    }
    
    #ifdef DEBUG_SEMA
    fprintf(stdout, "Sema Lock!\n");
    #endif

    return 1;
}
int unlock_sema(int id)
{
    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;

    if(semop(id, &buf, 1) == -1)
    {
        fprintf(stderr, "ERROR : Semaphore unlock fail");
        //announce kill message to other proccess
        setKillTrigger();
        // //kill this proccess immediately for prevent confliction of shared memory
        exit(-1);
    }

    #ifdef DEBUG_SEMA
    fprintf(stdout, "Sema UnLock!\n");
    #endif

    return 1;
}
/*******************************************************/


/***************** protocol section *******************/
int output_index[OUTPUT_DEVICE_NUM] =
{
    0, //LED
    2, //FND//Set Output Message(shared memory) from src (Only for cal_proc)
    7, //TEXT_LCD
    40 //DOT_MATRIX
};
size_t output_size[OUTPUT_DEVICE_NUM] =
{
    2, //LED
    5, //FND
    33, //TEXT_LCD
    11, //DOT_MATRIX
};
void set_output(const char* src, enum output_device_name output)
{
    lock_sema(output_message_sema_id);
    memcpy(vaddr_output_message + output_index[output], src, sizeof(unsigned char) * output_size[output]);
    unlock_sema(output_message_sema_id);
}
void get_output(char* dst)
{
    lock_sema(output_message_sema_id);
    memcpy(dst, vaddr_output_message, OUTPUT_MESSAGE_ELEM_SIZE);
    unlock_sema(output_message_sema_id);
}
void clear_output()
{
    lock_sema(output_message_sema_id);
    memset(vaddr_output_message, 0, OUTPUT_MESSAGE_ELEM_SIZE);
    vaddr_output_message[0] = RESET;
    unlock_sema(output_message_sema_id);    
}
/*******************************************************/

void checkOutputMessage()
{
    char local[50];
    get_output(local);
    int elem, elem_size = OUTPUT_MESSAGE_ELEM_SIZE/sizeof(unsigned char);
    printf("local : %s\n", local);
    for(elem=0;elem<elem_size;elem++)
        printf("(%d)",local[elem]);
    printf("\n");
}