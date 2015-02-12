/*
 * serial.c
 *
 *  Created on: Apr 4, 2013
 *      Author: Sergio Pino
 */

#include "serial.h"

int checkEven(float a) {

	if (a/2 - floorf(a/2))
		return 0; // false
	else
		return 1; // true
}

float mapping_21(float x1, float x2, float delta) {

	float theta = 0;

	// radius of the source point
	float rad = sqrtf(x1*x1 + x2*x2);

	float D = rad/delta;
	float F = floorf(D);

	// 1st quadrant
	if (x1>=0 && x2>=0) {

		float alpha = atanf(x2/x1);

		if ( checkEven(F) ) {

			if (rad > delta/PI*(alpha+F*PI+PI/2) ) {
				theta = alpha + F*PI+PI;
				theta = -theta;
			}
			else {

				if (F) {
					theta = alpha + F*PI;
				}
				else {
					float p2 = x2-delta/4;
					float beta = atanf(p2/x1);
					float m1 = delta/4*cosf(beta);
					float m2 = delta/4*sinf(beta)+delta/4;
					theta = atanf(m2/m1);
				}
			}
		}
		else {

			if (rad > delta/PI*(alpha + F*PI + PI/2) ) {
				theta = alpha + F*PI + PI;
			}
			else {
				theta = alpha + F*PI;
				theta = -theta;
			}
		}
	}

	// 2nd quadrant
	else if (x1<0 && x2>=0) {

		float alpha = atanf(x2/x1) + PI;
		if (checkEven(F)) {

			if (F==0 && x1>-delta/2 && x2<delta/4) {
				float p2 = x2 + delta/4;
				float beta = atanf(p2/x1);
				float m1 = -delta/4*cosf(beta);
				float m2 = -delta/4*sinf(beta)-delta/4;

				theta = atanf(m2/m1);
				theta = -theta;
			}
			else if ( rad < delta/PI*(alpha+(F-1)*PI + PI/2)) {
				theta = alpha + (F-1)*PI;
				theta = -theta;
			}
			else {
				theta = alpha + F*PI;
			}
		}
		else {

			if ( rad < delta/PI*(alpha+(F-1)*PI + PI/2)) {
				theta = alpha+(F-1)*PI;
			}
			else {
				theta = alpha + F*PI;
				theta = -theta;
			}
		}
	}

	// 3rd quadrant
	else if (x1<=0 && x2<=0) {
		float alpha = atanf(x2/x1);

		if (checkEven(F)) {

			if (rad > delta/PI*(alpha+F*PI + PI/2)) {
				theta = alpha+F*PI + PI;
			}
			else {

				if (F) {
					theta = alpha + F*PI;
					theta = -theta;
				}
				else {
					float p2 = x2 + delta/4;
					float beta = atanf(p2/x1);
					float m1 = -delta/4*cosf(beta);
					float m2 = -delta/4*sinf(beta) - delta/4;

					theta = atanf(m2/m1);
					theta = -theta;
				}
			}
		} else {
			if (rad > delta/PI*(alpha + F*PI + PI/2)) {
				theta = alpha + F*PI + PI;
				theta = -theta;
			}
			else {
				theta = alpha + F*PI;
			}
		}
	}

	// 4th quadrant
	else if (x1>0 && x2<=0) {
		float alpha = atanf(x2/x1) + PI;

		if (checkEven(F)) {

			if (F==0 && x1<delta/2 && x2>(-delta/4)) {
				float p2 = x2 - delta/4;
				float beta = atanf(p2/x1);
				float m1=delta/4*cosf(beta);
				float m2=delta/4*sinf(beta)+delta/4;
				theta = atanf(m2/m1);
			}
			else if (rad < delta/PI*(alpha+(F-1)*PI + PI/2)) {
				theta = alpha+(F-1)*PI;
			}
			else {
				theta = alpha + F*PI;
				theta = -theta;
			}
		}
		else {

			if (rad < delta/PI*(alpha + (F-1)*PI + PI/2)) {
				theta =alpha + (F-1)*PI;
				theta = -theta;
			}
			else {
				theta = alpha+ F*PI;
			}
		}
	}

	return theta;
}

float expAlpha(float theta, float alpha) {

	if (theta >= 0)
		return powf(theta, alpha);
	else
		return -powf(fabsf(theta), alpha);
}

// encoder using space-filling curves
void encoder_21(float *x_ptr,       // source symbols
		float *s_ptr,       // mapped symbols
		float *s_pw,    // average energy of mapped symbols
		float del,    // parameter delta
		float alp)    // parameter alpha
{

	*s_pw = 0;
	// Mapping the source symbols
	for(int i=0; i < CHN_LENGTH; i++)
	{
		float x1 = *(x_ptr + 2*i);
		float x2 = *(x_ptr + 2*i + 1);

		*(s_ptr + i) = expAlpha(mapping_21(x1, x2, del), alp);
		*s_pw += *(s_ptr + i) * *(s_ptr + i);
	}
	*s_pw /= CHN_LENGTH;
}

// transmission over AWGN channels
void channel(float *s_ptr, float *r_ptr, float s_pw, float *n_ptr) {
	// channel
	float s_pw_sqrt = sqrtf(s_pw);
	for(int i = 0;i < CHN_LENGTH;i++)
	{
		*(r_ptr + i) = *(s_ptr + i) + s_pw_sqrt * *(n_ptr + i);
	}
}

// demapping function, i.e. ML decoding
void ml_demapping_21(float r, // received symbol
				  float del, // parameter delta
				  float alp, // parameter alpha
				  float *xp_prt) {// decoded symbols

	float theta;
    if(r > 0) {
    	theta = powf(r, 1/alp);
        *(xp_prt) = del/PI*theta*cosf(theta);
        *(xp_prt + 1) = del/PI*theta*sinf(theta);
	}
    else {
        theta = -powf(fabsf(r), 1/alp);
        *(xp_prt) = del/PI*theta*cosf(fabsf(theta));
        *(xp_prt + 1) = del/PI*theta*sinf(fabsf(theta));
	}
}

// Maximum Likelihood Estimation decoder, return the output SDR
float ml_decoder_21(float *x_ptr, 	// source symbols
		float *r_ptr,   	// received channel symbols
		float *xml_ptr, 	// decoded sequence
		float delta,     // parameter delta
		float alpha)     // parameter alpha
{

	float ml_mmse=0;

	for(int i = 0; i < CHN_LENGTH; i++) {
		// ML decoding
		ml_demapping_21(*(r_ptr + i), delta, alpha, &xml_ptr[2*i]);

		// mean square error
		for(int j=0; j<2; j++)
			ml_mmse += powf(*(x_ptr + 2*i + j) - *(xml_ptr + 2*i + j), 2);
	}

	return ml_mmse;
}

void runSerial(int argc, char **argv,
		float *x_ptr, float x_energ,
		float *noise_ptr, int srclen, int chnlen) {

	printf("runSerial\n");

	SRC_LENGTH = srclen;
	CHN_LENGTH = chnlen;

	int i, j, k;

	float *n_ptr;
	float* alpha_ptr;
	float* delta_ptr;
	float ml_mmse;
	int n_alpha, n_delta;

	/*
	 * analyzing the arguments
	 *
	 * input: group_size STEP SRC srclen NOISE nlen CSNR0...CSNRN
	 *
	 */
	float* csnr_vec;
	int n_csnr_vec;
	float step = 0.1;

	if (argc > 7) {
		step = atof(argv[2]);
		n_csnr_vec = argc - 7;
		csnr_vec = (float*) malloc(n_csnr_vec * sizeof(float));

		for (int i = 7; i < argc; i++)
			csnr_vec[i - 7] = atof(argv[i]);

	} else {
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

	/*
	 * signal vectors
	 */
	// encoded symbols
	float* s_ptr = (float *) malloc(CHN_LENGTH * sizeof(float));
	// average power of the encoded symbols
	float s_pw;
	// received symbols after AWGN
	float *r_ptr = (float *) malloc(CHN_LENGTH * sizeof(float));
	// reconstructed symbols
	float *xml_ptr = (float *) malloc(SRC_LENGTH * sizeof(float));

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

		// optimizing the alpha and delta parameters
		float outputsdr[n_alpha][n_delta];
		int j_max_val = -1;
		int k_max_val = -1;
		float max_val = 0;

		// alpha
		for (j = 0; j < n_alpha; j++) {

			// delta
			for (k = 0; k < n_delta; k++) {

				// encoding 2:1
				encoder_21(x_ptr, s_ptr, &s_pw, delta_ptr[k], alpha_ptr[j]);
				// channel transmission
				channel(s_ptr, r_ptr, s_pw, n_ptr);

				// 1:2 Non Linear Analog Decoder ML
				ml_mmse = ml_decoder_21(x_ptr, r_ptr, xml_ptr, *(delta_ptr + k), *(alpha_ptr + j));// ml decoder

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
		printf("Serial clock(sec): %f, uSec: %g\n", clock_dif_sec, time);

		printf("CSNR = %f, max_sdr(%i, %i) = %f, alpha = %f, delta = %f\n",
				csnr_vec[i], j_max_val, k_max_val, outputsdr[j_max_val][k_max_val], *(alpha_ptr + j_max_val), *(delta_ptr + k_max_val));

	}

	printf("EO runSerial\n");
}


