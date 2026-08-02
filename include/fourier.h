#ifndef _FOURIER_H_
#define _FOURIER_H_

#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include "../include/fft.h"
#include "../include/eval.h"


typedef struct{
    float* an, *bn;
    float t0, T;
    int n;
}FourierSeriesCoefs;

FourierSeriesCoefs* ComputeFourierSeriesCoefs(char* func, float t0, float T, int n_harmonics);
float FourierSeries(FourierSeriesCoefs* fourier, float x, float freq);

#endif
