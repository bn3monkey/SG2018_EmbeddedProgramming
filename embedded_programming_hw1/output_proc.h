#ifndef __OUTPUT__PROC__
#define __OUTPUT__PROC__

#define OUTPUT_PROC 1002
#define DEBUG_OUTPUT

// For mmap
#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16



// Cycle Function of output
void output_proc();

// initialization of output function
// 1) shared_memory set
// 2) semaphore set
// 3) Device Open
//--output--
// 1 : success 0 : fail
int output_init();

// post process of output function
// 1) (X) shared_memory <-deallocation in cal_proc
// 2) (X) semaphore <-deallocation in cal_proc
// 3) Device Close
//--output--
// 1 : success 0 : fail
int output_deinit();


// Getting output of left side button and transfer to cal_proc
void output_side();
// Getting output of main button which decided by mode and transfer to cal_proc
void output_pad();
#endif