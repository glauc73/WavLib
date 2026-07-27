#include <malloc.h>
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static double complex* fft_(double* f, uint64_t N);
static double complex* ifft_(double complex* f, uint64_t N);
uint64_t next_pow2(uint64_t n);
double complex* fft(double* f, uint64_t* N);
double* ifft(double complex* F, uint64_t* n);
