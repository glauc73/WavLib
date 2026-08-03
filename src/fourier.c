#include "fourier.h"

FourierSeriesCoefs* ComputeFourierSeriesCoefs(char* func, float t0, float T, int n_harmonics){
    FourierSeriesCoefs* fourier = malloc(sizeof(*fourier));
    *fourier = (FourierSeriesCoefs){
        .T = T,
        .n = n_harmonics,
        .t0 = t0,
    };
    fourier->an = malloc(n_harmonics * sizeof(float));
    fourier->bn = malloc(n_harmonics * sizeof(float));

    unsigned min_nyquist = 2*n_harmonics;
    uint64_t M = next_pow2(min_nyquist);
    
    double* window_samples = malloc(M*sizeof(double));
    float pass = T/M;    
    for(int i = 0; i < M; i++){
        float yi = eval_X(func, t0 + pass * i);
        window_samples[i] = isfinite(yi) ? yi : 0.0f;
    }


    uint64_t N_fft = M;
    double complex* ft = fft(window_samples, &N_fft);

    fourier->an[0] = crealf(ft[0]) / M;
    fourier->bn[0] = 0.0f;
    for(int i = 1; i < n_harmonics; i++){
        double complex c_pos = ft[i];
        double complex c_neg = ft[N_fft - i];
        fourier->an[i] = (crealf(c_pos) + crealf(c_neg)) / M;
        fourier->bn[i] = (cimagf(c_neg) - cimagf(c_pos)) / M;
    }

    free(ft);
    free(window_samples);
    return fourier;
}

float FourierSeries(FourierSeriesCoefs* fourier, float x, float freq){
    float f_x_ = fourier->an[0];

    int N = fourier->n;
    for(int n = 1; n < N; n++){
        float t = x - fourier->t0;
        f_x_ += fourier->an[n] * cosf(2*M_PI*freq*n*t);
        f_x_ += fourier->bn[n] * sinf(2*M_PI*freq*n*t);
    }

    return f_x_;
}

