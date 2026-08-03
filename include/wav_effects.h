#ifndef _WAV_EFFECTS_H_
#define _WAV_EFFECTS_H_

#include "wav.h"

void wav_adjust_volume(wav_t* sound, float dB);
wav_t* wav_mix(const wav_t* sound1, const wav_t* sound2);
wav_t* wav_mix_array(wav_t** sounds, int n);
wav_t* wav_decay_exp(wav_t* sound, float k, bool inplace);
wav_t* wav_gaussian_window(wav_t* sound, float sigma, float mu, bool inplace);
wav_t* wav_tremble(wav_t* sound, float k, bool inplace);

#endif
