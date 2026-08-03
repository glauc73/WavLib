#ifndef _WAV_FILTERS_H_
#define _WAV_FILTERS_H_

#include "wav.h"
#include "fft.h"
#include <complex.h>

wav_t* wav_gaussian_bandpass(wav_t* sound, float fA, float fB, bool inplace);
wav_t* wav_gaussian_bandreject(wav_t* sound, float fA, float fB, bool inplace);
wav_t* wav_gaussian_lowpass(wav_t* sound, float fc, bool inplace);
wav_t* wav_gaussian_highpass(wav_t* sound, float fc, bool inplace);

#endif

