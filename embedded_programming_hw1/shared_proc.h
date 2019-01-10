#ifndef __SHARED_PROC__
#define __SHARED_PROC__

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define DEBUG_SHARED
#define DEBUG_KILLTRIGGER
//#define DEBUG_SEMA

/********************* Mode section ********************/
enum DEVICE_MODE
{
    MODE_CLOCK,
    MODE_COUNTER,
    MODE_TEXTEDITOR,
    MODE_DRAWBOARD,
    MODE_EXTRA,
    MODE_NUM,
};
/*******************************************************/

/*********** Data Transfer Protocol section ************/
//For Input Message Protocol
enum input_device_name
{
    INPUT_DEVICE_SIDE,
    INPUT_DEVICE_NUMPAD,
    INPUT_DEVICE_NUM
};
// Side input Protocol
enum side_input_name
{
    SIDE_RELEASED,
    SIDE_EXIT,
    SIDE_NEXTMODE,
    SIDE_PREVMODE,
    SIDE_INPUT_NUM,
};
//For Message Queue
//type distinction by using shared_memory_section definition.
typedef struct _queue_elem
{
    long mtype;
    char data[8];
} queue_elem;
#define NO_DATA 0

//For Output Message Protocol
enum output_device_name
{
    OUTPUT_DEVICE_LED,
    OUTPUT_DEVICE_FND,
    OUTPUT_DEVICE_TEXT,
    OUTPUT_DEVICE_DOT,
    OUTPUT_DEVICE_NUM
};
enum output_msg_prefix
{
    INVALID,
    VALID,
    RESET,
};
typedef struct _msg_led
{
    unsigned char prefix;
    unsigned char data;
} msg_led;
typedef struct _msg_fnd
{
    unsigned char prefix;
    unsigned char data[4];
} msg_fnd;
typedef struct _msg_text
{
    unsigned char prefix;
    unsigned char data[32];
} msg_text;
typedef struct _msg_dot
{
    unsigned char prefix;
    unsigned char data[10];
} msg_dot;
extern int output_index[OUTPUT_DEVICE_NUM];
extern size_t output_size[OUTPUT_DEVICE_NUM];


//if you want to use below function,
// you have to initialize output_message_id and output_message_sema_id

//Set Output Message(shared memory) from src (Only for cal_proc)
//--input--
// src : message which want to send
// output : output device name (enum output_name) 
void set_output(const char* src, enum output_device_name output);

//Get Output Message(shared memory) to dest (Only for output_proc)
//--input--
// src : message which want to receive
void get_output(char* dst);
// Set Output message(shared memory) to Reset
void clear_output();

/*******************************************************/

/**************** shared_memory section ****************/
#define CURRENT_MODE 3000 
#define INPUT_MODE_QUEUE 3001
#define INPUT_SWITCH_QUEUE 3002
#define OUTPUT_MESSAGE 3003
#define ALLKILL_TRIGGER 3004

#define QUEUE_SIZE 5
#define CURRENT_MODE_ELEM_SIZE (sizeof(int))
#define QUEUE_ELEM_SIZE (sizeof(queue_elem)-sizeof(long))
#define OUTPUT_MESSAGE_ELEM_SIZE (51*sizeof(unsigned char))
#define ALLKILL_TRIGGER_ELEM_SIZE (sizeof(int))

extern int current_mode_id;
extern int input_mode_queue_id;
extern int input_switch_queue_id;
extern int output_message_id;
extern int allkill_trigger_id;

extern int* vaddr_current_mode;
extern char* vaddr_output_message;
extern int* vaddr_allkill_trigger;

//Make and initializre new shared memory (only for main process) 
//--output--
// 1 : success 0 : fail
int make_shared_memory();
//Get existed shared memory (only for sub process)
//not needed to initialize because make_shared_memory() already initialize it
//--input--
//type : the species of shared memory
//--output--
// 1 : success 0 : fail
int get_shared_memory(int type);
//Delete shared memory (only for main process)
//--output--
// 1 : success 0 : fail
int delete_shared_memory();

// Confirm that another process is dead
// --output--
// 1 : another process is dead or will die, 0 : no process dies 
int getKillTrigger();

// Let Another Process know that process which call this function will die
void setKillTrigger();

// Debug to Output message
void checkOutputMessage();
/*******************************************************/

/***************** semaphore section *******************/
#define RESET_MASSAGE_SEMA 4001
#define OUTPUT_MESSAGE_SEMA 4002

extern int reset_message_sema_id;
extern int output_message_sema_id;


union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
};

//Make new semaphore (only for main process) 
//--output--
// 1 : success 0 : fail
int make_semaphore();
//Get existed semaphore (only for sub process)
//--input--
//type : the species of shared memory
//--output--
// 1 : success 0 : fail
int get_semaphore(int type);
//Delete semaphore (only for main process)
//--output--
// 1 : success 0 : fail
int delete_semaphore();

// Lock Critical Section
//--output--
// 1 : Success Kill this Process: Fail
int lock_sema(int id);
// Unlock Critial Section
//--output--
// 1 : Success Kill this Process: Fail
int unlock_sema(int id);


/*******************************************************/


#endif