/*
 * parallel.c
 *
 *  Created on: May 2, 2013
 *      Author: Sergio Pino
 *
 *  Reusing code from "A Gentle Introduction to OpenCL" and "OpenCL in Action" by Matthew Scarpino
 */

#include "parallel.h"

#define PROGRAM_FILE "skm.cl"
#define KERNEL_encode "encoder21"
#define KERNEL_DECODING "ml_decoder_21"

/* Find a GPU or CPU associated with the first available platform */
cl_device_id create_device() {

	cl_platform_id platform;
	cl_device_id dev;
	int err;

	/* Identify a platform */
	err = clGetPlatformIDs(1, &platform, NULL);
	if(err < 0) {
		perror("Couldn't identify a platform");
		exit(1);
	}

	/* Access a device */
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
	if(err == CL_DEVICE_NOT_FOUND) {
		printf("Couldn't get a GPU\n");
		err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
	}
	if(err < 0) {
		perror("Couldn't access any devices");
		exit(1);
	}

	cl_uint vector_width;
	clGetDeviceInfo(dev, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
			sizeof(vector_width), &vector_width, NULL);
	if(err < 0) {
		perror("Couldn't read device properties");
		exit(1);
	}
	printf("Preferred vector width in floats: %u\n", vector_width);
	clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
			sizeof(vector_width), &vector_width, NULL);
	if(err < 0) {
		perror("Couldn't read device properties");
		exit(1);
	}
	printf("Maximum number of dimensions: %u\n", vector_width);

	return dev;
}


/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename) {

	cl_program program;
	FILE *program_handle;
	char *program_buffer, *program_log;
	size_t program_size, log_size;
	int err;

	/* Read program file and place content into buffer */
	program_handle = fopen(filename, "r");
	if(program_handle == NULL) {
		perror("Couldn't find the program file");
		exit(1);
	}
	fseek(program_handle, 0, SEEK_END);
	program_size = ftell(program_handle);
	rewind(program_handle);
	program_buffer = (char*) malloc(program_size + 1);
	program_buffer[program_size] = '\0';
	fread(program_buffer, sizeof(char), program_size, program_handle);
	fclose(program_handle);

	/* Create program from file */
	program = clCreateProgramWithSource(ctx, 1,
			(const char**)&program_buffer, &program_size, &err);
	if(err < 0) {
		perror("Couldn't create the program");
		exit(1);
	}
	free(program_buffer);

	/* Build program */
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if(err < 0) {

		/* Find size of log and print to std output */
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
				0, NULL, &log_size);
		program_log = (char*) malloc(log_size + 1);
		program_log[log_size] = '\0';
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
				log_size + 1, program_log, NULL);
		printf("%s\n", program_log);
		free(program_log);
		exit(1);
	}

	return program;
}

void runParallel(int argc, char **argv,
		float *x_ptr, float x_energ,
		float *noise_ptr, int srclen, int chnlen) {

	printf("runParallel\n");

	SRC_LENGTH = srclen;
	CHN_LENGTH = chnlen;

	float *n_ptr;
	float* alpha_ptr;
	float* delta_ptr;
	float ml_mmse;
	int n_alpha, n_delta, group_size, i, j, k, l;
	float* csnr_vec;
	int n_csnr_vec;
	float step = 0.1;

	/* OpenCL structures */
	cl_device_id device;
	cl_context context;
	cl_program program;
	cl_kernel encoder21_kernel, decoder_kernel;
	cl_command_queue queue;
	cl_int err;
	size_t local_size, global_size;

	/* Buffers */
	cl_mem memN, memX, memS, memMLMMSE;

	/*
	 * analyzing the arguments
	 *
	 * input: group_size STEP SRC srclen NOISE nlen CSNR0...CSNRN
	 *
	 */
	if (argc > 7) {
		group_size = atoi(argv[1]);
		step = atof(argv[2]);
		n_csnr_vec = argc - 7;
		csnr_vec = (float*) malloc(n_csnr_vec * sizeof(float));

		for (int i = 7; i < argc; i++)
			csnr_vec[i - 7] = atof(argv[i]);

	} else {
		group_size = 128;
		n_csnr_vec = 1;
		csnr_vec = (float *) malloc(1 * sizeof(float));
		csnr_vec[0] = 30;
	}
	printf("step = %f, n_csnr_vec = %i\n", step, n_csnr_vec);

	// Alpha and Delta
	n_alpha = MAX_ALPHA/step;
	n_delta = MAX_DELTA/step;
	printf("n_alpha: %i\n", n_alpha);
	printf("n_delta: %i\n", n_delta);

	alpha_ptr = (float *) malloc (n_alpha * sizeof(float));
	delta_ptr = (float *) malloc (n_delta * sizeof(float));
	n_ptr = (float *) malloc (CHN_LENGTH * sizeof(float)); // noise with the expected variance

	// initializing alpha and delta
	for (j = 0; j < n_alpha; j++) {
		*(alpha_ptr + j) = step*(j+1);
	}
	for (j = 0; j < n_delta; j++) {
		*(delta_ptr + j) = step*(j+1);
	}

	/* Create device and determine local size */
	device = create_device();
	err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
			sizeof(local_size), &local_size, NULL);
	if(err < 0) {
		perror("Couldn't obtain device information");
		exit(1);
	}

	/* Create a context */
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if(err < 0) {
		perror("Couldn't create a context");
		exit(1);
	}

	/* Build program */
	program = build_program(context, device, PROGRAM_FILE);


	/*
	 * signal vectors
	 */
	// encoded symbols
	float* s_ptr = (float *) malloc(CHN_LENGTH * sizeof(float));
	// average power of the encoded symbols
	float s_pw;
	// sq of the s
	float *ml_mmse_ptr = (float *) malloc(SRC_LENGTH * sizeof(float));

	/* Create data buffer */

	// encoder21
	memX = clCreateBuffer(context, CL_MEM_READ_ONLY |
			CL_MEM_COPY_HOST_PTR, SRC_LENGTH * sizeof(float), x_ptr, &err);
	memS = clCreateBuffer(context, CL_MEM_READ_WRITE |
			CL_MEM_COPY_HOST_PTR, CHN_LENGTH * sizeof(float), s_ptr, &err);
	if(err < 0) {
		perror("Couldn't create a buffer");
		exit(1);
	};

	// decoding
	memMLMMSE = clCreateBuffer(context, CL_MEM_READ_WRITE |
			CL_MEM_COPY_HOST_PTR, SRC_LENGTH * sizeof(float), ml_mmse_ptr, &err);
	if(err < 0) {
		perror("Couldn't create a buffer");
		exit(1);
	};

	/* Create a command queue */
	queue = clCreateCommandQueue(context, device,
			CL_QUEUE_PROFILING_ENABLE, &err);
	if(err < 0) {
		perror("Couldn't create a command queue");
		exit(1);
	};

	/* Create a kernel */
	encoder21_kernel = clCreateKernel(program, KERNEL_encode, &err);
	if(err < 0) {
		perror("Couldn't create KERNEL_encode");
		exit(1);
	};

	/* Create kernels */
	decoder_kernel = clCreateKernel(program, KERNEL_DECODING, &err);
	if(err < 0) {
		perror("Couldn't create KERNEL_DECODING");
		exit(1);
	};

	/* Processing using the different problem size values*/
	for (i = 0; i < n_csnr_vec; i++) {
		printf("\nProcessing CSNR: %f...\n", csnr_vec[i]);

		/*
		 * Timing
		 */
		clock_t start_t, end_t, clock_dif; double clock_dif_sec;
		start_t = clock();
		struct timeval begin, end;
		gettimeofday (&begin, NULL);

		float sigman = 1/sqrtf( powf(10, *(csnr_vec + i)/10) ); // CSNR in natural

		// generate noise
		for (j = 0; j < CHN_LENGTH; j++) {
			*(n_ptr + j) = *(noise_ptr + j) * sigman;
		}

		memN = clCreateBuffer(context, CL_MEM_READ_ONLY |
				CL_MEM_COPY_HOST_PTR, CHN_LENGTH * sizeof(float), n_ptr, &err);

		// optimizing the alpha and delta parameters
		float outputsdr[n_alpha][n_delta];
		int j_max_val = -1;
		int k_max_val = -1;
		float max_val = 0;

		global_size = CHN_LENGTH;
		printf("global_size: %zu, local_size: %zu, num_groups: %zu\n", global_size, local_size, global_size/local_size);
		// alpha
		for (j = 0; j < n_alpha; j++) {

			// delta
			for (k = 0; k < n_delta; k++) {

				/* Create kernel arguments */

				// encoder21_kernel
				err = clSetKernelArg(encoder21_kernel, 0, sizeof(cl_mem), &memX);
				err |= clSetKernelArg(encoder21_kernel, 1, sizeof(cl_mem), &memS);
				err |= clSetKernelArg(encoder21_kernel, 2, sizeof(float), &delta_ptr[k]);
				err |= clSetKernelArg(encoder21_kernel, 3, sizeof(float), &alpha_ptr[j]);
				if(err < 0) {
					perror("Couldn't create a kernel argument");
					exit(1);
				}

				/* Enqueue kernel */
				err = clEnqueueNDRangeKernel(queue, encoder21_kernel, 1, NULL, &global_size,
						&local_size, 0, NULL, NULL);
				if(err < 0) {
					perror("Couldn't enqueue the encoder21_kernel");
					exit(1);
				}

				/* Finish processing the queue and get profiling information */
				clFinish(queue);

				err = clEnqueueReadBuffer(queue, memS, CL_TRUE, 0,
						CHN_LENGTH * sizeof(float), s_ptr, 0, NULL, NULL);
				if(err < 0) {
					perror("Couldn't read the buffer");
					exit(1);
				}

				// Reduction in CPU
				s_pw = 0.0f;
				for(l = 0; l < CHN_LENGTH; l++)
					s_pw += *(s_ptr + l) * *(s_ptr + l);
				s_pw /= CHN_LENGTH;

				float s_pw_sqrt = sqrt(s_pw);
				// decoding_kernel
				err = clSetKernelArg(decoder_kernel, 0, sizeof(cl_mem), &memX);
				err |= clSetKernelArg(decoder_kernel, 1, sizeof(cl_mem), &memS);
				err |= clSetKernelArg(decoder_kernel, 2, sizeof(cl_mem), &memN);
				err |= clSetKernelArg(decoder_kernel, 3, sizeof(cl_mem), &memMLMMSE);
				err |= clSetKernelArg(decoder_kernel, 4, sizeof(float), &s_pw_sqrt);
				err |= clSetKernelArg(decoder_kernel, 5, sizeof(float), &delta_ptr[k]);
				err |= clSetKernelArg(decoder_kernel, 6, sizeof(float), &alpha_ptr[j]);
				if(err < 0) {
					perror("Couldn't create a kernel argument");
					exit(1);
				}
				/* Enqueue kernel */
				err = clEnqueueNDRangeKernel(queue, decoder_kernel, 1, NULL, &global_size,
						&local_size, 0, NULL, NULL);
				if(err < 0) {
					perror("Couldn't enqueue the encoder21_kernel");
					exit(1);
				}

				/* Finish processing the queue and get profiling information */
				clFinish(queue);

				err = clEnqueueReadBuffer(queue, memMLMMSE, CL_TRUE, 0,
						SRC_LENGTH * sizeof(float), ml_mmse_ptr, 0, NULL, NULL);
				if(err < 0) {
					perror("Couldn't read the buffer");
					exit(1);
				}

				// Reduction in CPU
				ml_mmse = 0.0f;
				for(l = 0; l < SRC_LENGTH; l++)
					ml_mmse += *(ml_mmse_ptr + l);

				// SDR
				outputsdr[j][k] = 10 * log10f(x_energ * SRC_LENGTH / ml_mmse);
				if (outputsdr[j][k] > max_val ||
						(j_max_val == -1 &&	k_max_val == -1)) {
					j_max_val = j;
					k_max_val = k;
					max_val = outputsdr[j][k];
				}
			}
		}

		gettimeofday (&end, NULL);
		/* average time */
		end_t = clock(); clock_dif = end_t - start_t;
		clock_dif_sec = (double) (clock_dif/(1000000.0));
		double time = 1e6*(end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec);
		printf("Parallel-1 clock(sec): %f, uSec: %g\n", clock_dif_sec, time);

		printf("CSNR = %f, max_sdr(%i, %i) = %f, alpha = %f, delta = %f\n",
				csnr_vec[i], j_max_val, k_max_val, outputsdr[j_max_val][k_max_val], *(alpha_ptr + j_max_val), *(delta_ptr + k_max_val));
	}

	/* Deallocate resources */
	free(s_ptr);
	clReleaseKernel(encoder21_kernel);
	clReleaseKernel(decoder_kernel);
	clReleaseMemObject(memX);
	clReleaseMemObject(memS);
	clReleaseMemObject(memN);
	clReleaseMemObject(memMLMMSE);
	clReleaseCommandQueue(queue);
	clReleaseProgram(program);
	clReleaseContext(context);

	printf("EO runParallel\n");
}
