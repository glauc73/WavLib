#define _USE_MATH_DEFINES
#include "../include/fft.h"


static double complex* fft_(double* f, uint64_t N){
    if(N == 1){
        double complex* r = malloc(sizeof(double complex));
        r[0] = f[0];
        return r;
    }
    uint64_t M = N/2;
    double* f_even = malloc(M*sizeof(double));
    double* f_odd = malloc(M*sizeof(double));
    for(int i = 0; i < M; i++){
        f_even[i] = f[2*i];
        f_odd[i] = f[2*i + 1];
    }

    double complex* F_even = fft_(f_even, M);
    double complex* F_odd = fft_(f_odd, M);

    double complex* F = malloc(N * sizeof(double complex));
    for(int u = 0; u < M; u++){
        double complex W = cexp(-2.0f*I*M_PI*u / N);
        F[u] = F_even[u] + W*F_odd[u];
        F[u + M] = F_even[u] - W*F_odd[u];
    }

    free(F_even);
    free(F_odd);
    free(f_even);
    free(f_odd);

    return F;
}

static complex double* ifft_(double complex* f, uint64_t N){
    if(N == 1){
        complex double* r = malloc(sizeof(double complex));
        r[0] = f[0];
        return r;
    }
    uint64_t M = N/2;
    double complex* f_even = malloc(M*sizeof(double complex));
    double complex* f_odd = malloc(M*sizeof(double complex));
    for(int i = 0; i < M; i++){
        f_even[i] = f[2*i];
        f_odd[i] = f[2*i + 1];
    }

    complex double* F_even = ifft_(f_even, M);
    complex double* F_odd = ifft_(f_odd, M);

    complex double* F = malloc(N * sizeof(double complex));
    for(int u = 0; u < M; u++){
        double complex W = cexp(2.0f*I*M_PI*u / N);
        F[u] = (F_even[u] + W*F_odd[u]);
        F[u + M] = F_even[u] - W*F_odd[u];
    }

    free(F_even);
    free(F_odd);
    free(f_even);
    free(f_odd);

    return F;
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

double complex* fft(double* f, uint64_t *N){
    uint64_t new_N = next_pow2(*N);
    double *buff = calloc(new_N,  sizeof(double));
    if(!buff) return NULL;
    memcpy(buff, f, (*N)*sizeof(double));

    double complex* F = fft_(buff, new_N);
    free(buff);
    *N = new_N;
    return F;
}

double* ifft(double complex* F, uint64_t* n){
    uint64_t new_n = next_pow2(*n);
    double complex *buff = calloc(new_n, sizeof(double complex));
    if(!buff) return NULL;
    memcpy(buff, F, (*n)*sizeof(double complex));

    double complex* f = ifft_(buff, new_n);
    free(buff);

    double* f_flt = malloc(sizeof(double) * new_n);
    if(!f_flt) {free(f); return NULL;}

    for(uint64_t i = 0; i < new_n; i++){
        f_flt[i] = creal(f[i]) / (double)new_n;
    }
    free(f);
    
    *n = new_n;
    return f_flt;
}

