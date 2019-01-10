#ifndef __INPUT__PROC__
#define __INPUT__PROC__

#define INPUT_PROC 1000
#define DEBUG_INPUT

#define BUFF_SIZE 64
#define MAX_BUTTON 9
#define KEY_RELEASE 0
#define KEY_PRESSED 1

// Cycle Function of input
void input_proc();

// initialization of input function
// 1) shared_memory set
// 2) semaphore set
// 3) Device Open
//--output--
// 1 : success 0 : fail
int input_init();

// post process of input function
// 1) (X) shared_memory <-deallocation in cal_proc
// 2) (X) semaphore <-deallocation in cal_proc
// 3) Device Close
//--output--
// 1 : success 0 : fail
int input_deinit();


// Getting input of left side button and transfer to cal_proc
void input_side();
// Getting input of main button which decided by mode and transfer to cal_proc
void input_pad();


#endif