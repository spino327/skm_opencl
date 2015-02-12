/*
 * serial.h
 *
 *  Created on: Apr 4, 2013
 *      Author: Sergio Pino
 */

#ifndef SERIAL_H_
#define SERIAL_H_

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

int checkEven(float a);

float mapping_21(float x1, float x2, float delta);

void encoder_21(float *x_ptr,       // source symbols
		float *s_ptr,       // mapped symbols
		float *s_pw,    // average energy of mapped symbols
		float del,    // parameter delta
		float alp);    // parameter alpha

void channel(float *s_ptr, // symbols to be transmitted through AWGN channels
		float *r_ptr, // symbols after being transmitted through AWGN channels
		float s_pw, // average power of channel symbols (y)
		float *n_ptr); // channel noises

void ml_demapping_21(float r, // received symbol
		float del, // parameter delta
		float alp, // parameter alpha
		float *xp_prt); // decoded symbols

// Maximum Likelihood Estimation decoder, return the output SDR
float ml_decoder_21(float *x_ptr, 	// source symbols
		float *r_ptr,   	// received channel symbols
		float *xml_ptr, 	// decoded sequence
		float delta,     // parameter delta
		float alpha);     // parameter alpha

void runSerial(int argc, char **argv,
		float *x_ptr, float x_energ,
		float *noise_ptr, int srclen, int chnlen);

#endif /* SERIAL_H_ */
