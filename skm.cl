#define PI 3.1415927

int checkEven(float a) {

	if (a/2 - floor(a/2))
		return 0; // false
	else
		return 1; // true
}

float mapping_21(float x1, float x2, float delta) {

	float theta = 0;

	// radius of the source point
	float rad = sqrt(x1*x1 + x2*x2);

	float D = rad/delta;
	float F = floor(D);

	// 1st quadrant
	if (x1>=0 && x2>=0) {

		float alpha = atan(x2/x1);

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
					float beta = atan(p2/x1);
					float m1 = delta/4*cos(beta);
					float m2 = delta/4*sin(beta)+delta/4;
					theta = atan(m2/m1);
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

		float alpha = atan(x2/x1) + PI;
		if (checkEven(F)) {

			if (F==0 && x1>-delta/2 && x2<delta/4) {
				float p2 = x2 + delta/4;
				float beta = atan(p2/x1);
				float m1 = -delta/4*cos(beta);
				float m2 = -delta/4*sin(beta)-delta/4;

				theta = atan(m2/m1);
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
		float alpha = atan(x2/x1);

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
					float beta = atan(p2/x1);
					float m1 = -delta/4*cos(beta);
					float m2 = -delta/4*sin(beta) - delta/4;

					theta = atan(m2/m1);
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
		float alpha = atan(x2/x1) + PI;

		if (checkEven(F)) {

			if (F==0 && x1<delta/2 && x2>(-delta/4)) {
				float p2 = x2 - delta/4;
				float beta = atan(p2/x1);
				float m1=delta/4*cos(beta);
				float m2=delta/4*sin(beta)+delta/4;
				theta = atan(m2/m1);
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
		return pow(theta, alpha);
	else
		return -pow(fabs(theta), alpha);
}

// demapping function, i.e. ML decoding
void ml_demapping_21(float r, // received symbol
				  float del, // parameter delta
				  float alp, // parameter alpha
				  float *x1,
				  float *x2) {// decoded symbols

	float theta;
    if(r > 0) {
    	theta = pow(r, 1/alp);
        *(x1) = del/PI*theta*cos(theta);
        *(x2) = del/PI*theta*sin(theta);
	}
    else {
        theta = -pow(fabs(r), 1/alp);
        *(x1) = del/PI*theta*cos(fabs(theta));
        *(x2) = del/PI*theta*sin(fabs(theta));
	}
}

// Maximum Likelihood Estimation decoder, return the output SDR
__kernel void ml_decoder_21(__global float *x_ptr, 	// source symbols
		__global float *s_ptr, // channel symbols
		__global float *n_ptr, // noise
		__global float *ml_mmse,
		float s_pw_sqrt,
		float delta,     // parameter delta
		float alpha)     // parameter alpha
{
	
	float x1, x2, r;
	uint threadId, global_addr;
	
	threadId = get_global_id(0);
	global_addr = threadId * 2;

	// channel
	r = s_ptr[threadId] + s_pw_sqrt * n_ptr[threadId];
	
	// ML decoding
	ml_demapping_21(r, delta, alpha, &x1, &x2);

	// mean square error
	ml_mmse[global_addr] = pow(x_ptr[global_addr] - x1, 2);
	ml_mmse[global_addr + 1] = pow(x_ptr[global_addr + 1] - x2, 2);
}

// generate noise
__kernel void generate_noise(__global float* noise_ptr, __global float* n_ptr, float sigman, int max)
{
	int threadId = get_global_id(0);

	if (threadId < max)
		n_ptr[threadId] = noise_ptr[threadId] * sigman;
}

__kernel void encoder21(__global float* x_ptr, __global float* s_ptr, float del, float alp)
{
	// encoding 2:1
	float x1, x2, s;
	uint threadId, global_addr; 

	threadId = get_global_id(0);
	global_addr = threadId * 2;

	x1 = x_ptr[global_addr];
	x2 = x_ptr[global_addr + 1];

	s = expAlpha(mapping_21(x1, x2, del), alp);

	s_ptr[threadId] = s;
}