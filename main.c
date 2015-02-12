/*
 * main.c
 *
 *  Created on: May 2, 2013
 *      Author: Sergio Pino
 */
#include "serial.h"
#include "parallel.h"

void loadData(char* fn, int size, float* data_ptr, float* mean_ptr, float* sq_ptr) {

	if (fn == NULL)
		return;

	int i;
	FILE *inFile;

	inFile = fopen(fn, "r");
	if (inFile == NULL) {
		printf("Unable to open file\n");
		exit(1);
	}

	// the energy of the source
	float sum_sq = 0;
	float sum_mean = 0;
	int count = 0;

	for(i = 0; i < size; i++) {
		fscanf(inFile, "%f",&*(data_ptr + i));

		sum_mean += *(data_ptr + i);
		sum_sq += *(data_ptr + i) * *(data_ptr + i);

		count++;
	}
	fclose(inFile);

	printf("Values read: %i\n", count);

	*mean_ptr = sum_mean/size;
	*sq_ptr = sum_sq/size;
}

/* This will run the serial and parallel code*/
int main (int argc, char **argv) {

	/*
	 * analyzing the arguments
	 *
	 * input: group_size STEP SRC srclen NOISE nlen CSNR0...CSNRN
	 *
	 */
	char* src;
	char* noise;
	int SRC_LENGTH, CHN_LENGTH;

	if (argc > 7) {
		src = argv[3];
		SRC_LENGTH = atoi(argv[4]);
		noise = argv[5];
		CHN_LENGTH = atoi(argv[6]);
	} else {
		exit(1);
	}

	/*
	 * Source and noise
	 */
	// source
	float *x_ptr = (float*) malloc(SRC_LENGTH * sizeof(float));

	// the energy of the source
	float x_mean;
	float x_energ;

	loadData(src, SRC_LENGTH, x_ptr, &x_mean, &x_energ);

	printf("Mean = %f, var = %f, Energy = %f\n", x_mean, x_energ - x_mean*x_mean, x_energ);

	// noise
	float *noise_ptr = (float*) malloc(CHN_LENGTH * sizeof(float));

	// the energy of the source
	float noise_energ;
	float noise_mean;

	loadData(noise, CHN_LENGTH, noise_ptr, &noise_mean, &noise_energ);

	printf("Mean = %f, var = %f, Energy = %f\n", noise_mean, noise_energ - noise_mean*noise_mean, noise_energ);


	/*
	 * Serial
	 */
	runSerial(argc, argv, x_ptr, x_energ, noise_ptr, SRC_LENGTH, CHN_LENGTH);

	/*
	 * Parallel
	 */
	runParallel(argc, argv, x_ptr, x_energ, noise_ptr, SRC_LENGTH, CHN_LENGTH);

	return 0;
}
