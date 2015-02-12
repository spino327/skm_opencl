/*
 * parallel.h
 *
 *  Created on: May 2, 2013
 *      Author: Sergio Pino
 */

#ifndef PARALLEL_H_
#define PARALLEL_H_

// define parameters
#define PI 3.1415927
#define MAX_ALPHA 2
#define MAX_DELTA 100

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/param.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>

#include <assert.h>
#include <xmmintrin.h>

int SRC_LENGTH, CHN_LENGTH;

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


/* Find a GPU or CPU associated with the first available platform */
cl_device_id create_device();

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename);

void runParallel(int argc, char **argv,
		float *x_ptr, float x_energ,
		float *noise_ptr, int srclen, int chnlen);

#endif /* PARALLEL_H_ */
