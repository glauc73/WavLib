#ifndef _WAV_H_
#define _WAV_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "wav_error.h"

#define MAX_AMPLITUDE_16BIT (short)0x7fff
#define MIN_AMPLITUDE_16BIT (short)0x8000
#define tostr(x) #x


#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

typedef struct {
    char chunk_id[4];        // "RIFF"
    unsigned chunk_size;         // 36 + subchunk2_size
    char format[4];          // "WAVE"

    char subchunk1_id[4];    // "fmt "
    unsigned subchunk1_size;    // 16
    short audio_format;     // 1 (PCM)
    short num_channels;      // 1 (Mono) / 2 (Stereo)
    unsigned sample_rate;       // Ex: 44100
    unsigned byte_rate;     //sample_rate * bits_per_sample/8
    short block_align; // num_channels * bits_per_sample / 8
    short bits_per_sample;   // 16
    //
    char subchunk2_id[4];    // "data"
    unsigned subchunk2_size; //num_frames * num_channels * bits_per_sample / 8
} WavHeader;

#define STD_HEADER(rate, channels, num_samples) ((WavHeader){ \
    .chunk_id       = {'R', 'I', 'F', 'F'}, \
    .chunk_size     = 36 + ((num_samples) * (channels) * sizeof(short)), \
    .format         = {'W', 'A', 'V', 'E'}, \
    .subchunk1_id   = {'f', 'm', 't', ' '}, \
    .subchunk1_size = 16, \
    .audio_format   = 1, /* PCM */ \
    .num_channels   = (channels), \
    .sample_rate    = (rate), \
    .byte_rate      = (rate) * (channels) * sizeof(short), \
    .block_align    = (channels) * sizeof(short), \
    .bits_per_sample= 16, \
    .subchunk2_id   = {'d', 'a', 't', 'a'}, \
    .subchunk2_size = (num_samples) * sizeof(short) \
})

typedef struct {
    WavHeader header;
    int16_t *data;
    
    unsigned num_samples;    
    unsigned num_frames;        
    float time_duration;        
} wav_t;

wav_t* wav_new(WavHeader h);
wav_t* wav_read(const char* file_name);
void wav_write(const wav_t* sound, const char* file_name);
wav_t* wav_copy(const wav_t* sound);
void wav_delete(wav_t** sound);
wav_t* wav_to_mono(wav_t* sound, bool inplace);
wav_t* wav_resample(wav_t* sound, unsigned new_sample_rate, bool inplace);
int wav_norm_max(wav_t* sound);

#endif
