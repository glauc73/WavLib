#ifndef _FFT_H_
#define _FFT_H_

#include <malloc.h>
#include <complex.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum{TIME, FREQUENCY} fft_domain_t;

#define fft_at(ft) ft->domain == TIME ? ft->time : ft->freq

typedef struct{
    float complex* data;
    size_t N, n_samples;
    fft_domain_t domain;
}fft_t;

int64_t reverse_bit(uint64_t i, int n);
uint64_t next_pow2(uint64_t n);

fft_t* fft_new(size_t N, fft_domain_t dom);
fft_t* fft_init_from_time(float* data, size_t n_data, size_t N);
fft_t* fft_init_from_freq(float complex* data, size_t n, size_t N);
void fft_delete(fft_t** FT);
void fft_run(fft_t* ft);
void ifft_run(fft_t* ft);

#endif
