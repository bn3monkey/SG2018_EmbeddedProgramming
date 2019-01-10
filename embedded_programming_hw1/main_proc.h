#ifndef __MAIN_PROC__
#define __MAIN_PROC__

#include "input_proc.h"
#include "cal_proc.h"
#include "output_proc.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

extern pid_t pid_input;
extern pid_t pid_output;
#define isCalProcess (pid_input && pid_output)
#define isInputProcess (!pid_input)
#define isOutputProcess (!pid_output)

#endif