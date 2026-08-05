#include <complex.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include "fft.h"

fft_t* fft_new(size_t N, fft_domain_t dom){
    fft_t* FT = malloc(sizeof(fft_t));
    FT->n_samples = N;
    FT->domain = dom;
    FT->N = next_pow2(N);
    FT->data = calloc(FT->N, sizeof(float complex));
    return FT;
}

fft_t* fft_init_from_time(float* data, size_t n_data, size_t N){
    if(n_data > N) N = n_data;

    fft_t* ft = fft_new(N, TIME);
    if(!ft) return NULL;

    ft->n_samples = n_data;
    for(int i = 0; i < n_data; i++)
        ft->data[i] = data[i];

    return ft;
}

fft_t* fft_init_from_freq(float complex* data, size_t n_data, size_t N){
    if(n_data > N) N = n_data;

    fft_t* ft = fft_new(N, FREQUENCY);
    if(!ft) return NULL;

    ft->n_samples = n_data;
    memcpy(ft->data, data, n_data*sizeof(float complex));
    return ft;
}


void fft_delete(fft_t** FT){
    if(!FT || !*FT) return;
    free((*FT)->data);
    free(*FT);
    *FT = NULL;
}

uint64_t next_pow2(uint64_t n){
    if(n <= 1) return 1;
    
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

int64_t reverse_bit(uint64_t i, int n){
    short i_bit = 0;

    int64_t inv_bit = 0;
    for(int64_t j = n - 1; j >= 0; j--){
        inv_bit |= ((i >> j) & 1ULL) << (n - j - 1);
    }
    return inv_bit;
}

void fft_run(fft_t* ft){
    if(!ft || !ft->data || ft->domain == FREQUENCY) return;
    size_t N = ft->N;
    
    float complex* X = calloc(N, sizeof(float complex)); 
    float complex* x = ft->data;

    uint8_t log2n = 0;
    while(N > 1) {N >>= 1; log2n++;}
    N = ft->N;

    for(int i = 0; i < N; i++){
        uint64_t rev = reverse_bit(i, log2n);
        X[rev] = x[i];  
    }
    
    for(size_t M = 2; M <= N; M <<= 1){
        float complex w = cexpf(-2*M_PI*I/M);

        for(int i = 0; i < N; i += M){
            float complex W = 1.0;

            for(int j = 0; j < M/2; j++){
                float complex u = X[i + j];
                float complex v = X[i + j + M/2];

                X[i + j] = u + v*W;
                X[i + j + M/2] = u - v*W;

                W *= w;
            }
        }
    }
    
    free(ft->data);
    ft->data = X;
    ft->domain = FREQUENCY;
}

void ifft_run(fft_t* ft){
    if(!ft || !ft->data || ft->domain == TIME) return;
    size_t N = ft->N;
    
    float complex* x = calloc(N, sizeof(float complex)); 
    float complex* X = ft->data;

    uint8_t log2n = 0;
    while(N > 1) {N >>= 1; log2n++;}
    N = ft->N;

    for(int i = 0; i < N; i++){
        uint64_t rev = reverse_bit(i, log2n);
        x[rev] = X[i];  
    }
    
    for(size_t M = 2; M <= N; M <<= 1){
        float complex w = cexpf(2*M_PI*I/M);

        for(int i = 0; i < N; i += M){
            float complex W = 1.0;

            for(int j = 0; j < M/2; j++){
                float complex u = x[i + j];
                float complex v = x[i + j + M/2];

                x[i + j] = u + v*W;
                x[i + j + M/2] = u - v*W;

                W *= w;
            }
        }
    }

    free(ft->data);
    ft->data = x;
    
    for(int i = 0; i < N; i++){ 
        ft->data[i] = x[i]/N;
    }
    ft->domain = TIME;
}
