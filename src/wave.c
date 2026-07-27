#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include "../include/eval.h"

#define MAX_AMPLITUDE_16BIT (short)0x7fff
#define MIN_AMPLITUDE_16BIT (short)0x8000

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
    void *data;
    
    unsigned num_samples;    
    unsigned num_frames;        
    float time_duration;        
} wav_t;

wav_t* wav_new(WavHeader h){
    wav_t* sound = malloc(sizeof(wav_t));
    if(!sound) return NULL;

    sound->header = h;
    sound->data = malloc(h.subchunk2_size);
    if(!sound->data) return NULL;
    sound->num_samples = sound->header.subchunk2_size/(sound->header.bits_per_sample / 8);
    sound->num_frames = sound->num_samples / sound->header.num_channels;
    sound->time_duration = (float)sound->num_frames / (float)sound->header.sample_rate;
    return sound;
}


wav_t* wav_read(const char* file_name){
    char* ext = strrchr(file_name, '.');
    if(!ext || strcmp(ext, ".wav") != 0){
        printf("Só é possível manipular arquivos com extensão .wav\n");
        exit(EXIT_FAILURE);
    }
    FILE* f = fopen(file_name, "rb");
    if(f == NULL){   
        printf("O arquivo '%s' não existe no diretório atual\n", file_name);
        exit(EXIT_FAILURE);
    }

    wav_t* sound = malloc(sizeof(wav_t));

    fread(&(sound->header), sizeof(WavHeader), 1, f);
    sound->data = malloc(sound->header.subchunk2_size);
    
    fread(sound->data, 1, sound->header.subchunk2_size, f);
    fclose(f);

    sound->num_samples = sound->header.subchunk2_size/(sound->header.bits_per_sample / 8);
    sound->num_frames = sound->num_samples / sound->header.num_channels;
    sound->time_duration = (float)sound->num_frames / (float)sound->header.sample_rate;

    return sound;
}

void wav_write(wav_t* sound, const char* file_name){
    if(sound == NULL) return;
    char* ext = strrchr(file_name, '.');
    if(!ext || strcmp(ext, ".wav") != 0){
        printf("Só é possível manipular arquivos com extensão .wav\n");
        exit(EXIT_FAILURE);
    }
    FILE* f = fopen(file_name, "wb");

    fwrite(&(sound->header), sizeof(WavHeader), 1, f);
    fwrite(sound->data, 1, sound->header.subchunk2_size, f);

    fclose(f);
}

wav_t* wav_copy(wav_t* sound){
    if(!sound) return NULL;
    wav_t* cp = malloc(sizeof(wav_t));
    *cp = *sound;
    cp->data = malloc(cp->header.subchunk2_size);
    memcpy(cp->data, sound->data, cp->header.subchunk2_size);
    return cp;
}

void wav_delete(wav_t** sound){
    if(sound == NULL || *sound == NULL) return;

    if((*sound)->data) free((*sound)->data);
    free(*sound);
    *sound = NULL;
}

void wav_adjust_volume(wav_t* sound, float dB){
    float k = powf(10.0f, dB/20.0f);
    short* samples = sound->data;
    short max = abs(samples[0]);
    for(int i = 1; i < sound->num_samples; i++){
        if(abs(samples[i]) > max) max = abs(samples[i]);
    }
    float factor = k;
    if(max * k > MAX_AMPLITUDE_16BIT*0.95f)
        factor = (MAX_AMPLITUDE_16BIT*0.95f) / max;

    for(int i = 0; i < sound->num_samples; i++){
        samples[i] = (short) (samples[i] * factor);
    }
}

wav_t* wav_mix(wav_t* sound1, wav_t* sound2){
    if (!sound1 && !sound2) return NULL;
    if (!sound1) return wav_copy(sound2);
    if (!sound2) return wav_copy(sound1);

    if (sound1->header.sample_rate != sound2->header.sample_rate) {
        printf("Erro [wav_mix]: Taxas de amostragem incompatíveis (%u Hz vs %u Hz).\n", 
               sound1->header.sample_rate, sound2->header.sample_rate);
        return NULL;
    }
    if (sound1->header.num_channels != sound2->header.num_channels) {
        printf("Erro [wav_mix]: Número de canais incompatível (%d ch vs %d ch).\n", 
               sound1->header.num_channels, sound2->header.num_channels);
        return NULL;
    }
    if (sound1->header.bits_per_sample != sound2->header.bits_per_sample) {
        printf("Erro [wav_mix]: bits por amostra incompatíveis (%d bits vs %d bits).\n", 
               sound1->header.bits_per_sample, sound2->header.bits_per_sample);
        return NULL;
    }
    wav_t* greater_sound = sound1->num_samples > sound2->num_samples ? sound1 : sound2;
    wav_t* less_sound = sound1->num_samples < sound2->num_samples ? sound1 : sound2;
    wav_t* mix_sound = wav_copy(greater_sound);

    short* samples1 = sound1->data, *samples2 = sound2->data;
    
    int max1 = abs(samples1[0]), max2 = abs(samples2[0]);
    for(int i = 1; i < sound1->num_samples; i++)
        if(abs(samples1[i]) > max1) max1 = abs(samples1[i]);
    for(int i = 1; i < sound2->num_samples; i++)
        if(abs(samples2[i]) > max2) max2 = abs(samples2[i]);
        
    float factor = 1.0f;
    if(max1 + max2 > MAX_AMPLITUDE_16BIT*0.95f)
        factor = (MAX_AMPLITUDE_16BIT*0.95f) / (max1 + max2);

    for(int i = 0; i < less_sound->num_samples; i++){
        int mixed_int = (int)(( - samples1[i] + samples2[i]) * factor);
        ((short*)mix_sound->data)[i] = (short)mixed_int;
    }

    for(int i = less_sound->num_samples; i < greater_sound->num_samples; i++){
        ((short*)mix_sound->data)[i] *= factor;
    }

    return mix_sound;
}

wav_t* wav_mix_array(wav_t** sounds, int n){
    if (!sounds) return NULL;
    wav_t* first_sound = NULL;
    int valid_sound = 0;

    for(int i = n - 1; i >= 0; i--){
        if(sounds[i] != NULL) {
            first_sound = sounds[i];
            valid_sound++;
        }
    }

    if(valid_sound == 0) return NULL;
    if(valid_sound == 1) return wav_copy(first_sound);

    for(int i = 0; i < n; i++){
        if(!sounds[i] || !sounds[i]->data) continue;

        if (sounds[i]->header.sample_rate != first_sound->header.sample_rate) {
            printf("Erro [wav_mix]: Taxas de amostragem incompatíveis (%u Hz vs %u Hz).\n",
                sounds[i]->header.sample_rate, first_sound->header.sample_rate);
            return NULL;
        }
        if (sounds[i]->header.num_channels != first_sound->header.num_channels) {
            printf("Erro [wav_mix]: Número de canais incompatível (%d ch vs %d ch).\n",
                sounds[i]->header.num_channels,first_sound->header.num_channels);
            return NULL;
        }
        if (sounds[i]->header.bits_per_sample != first_sound->header.bits_per_sample) {
            printf("Erro [wav_mix]: bits por amostra incompatíveis (%d bits vs %d bits).\n",
                sounds[i]->header.bits_per_sample, first_sound->header.bits_per_sample);
            return NULL;
        }
    }
    wav_t* greater_sound = first_sound;
    for(int i = 0; i < n; i++){
        if(!sounds[i] || !sounds[i]->data) continue;

        if(sounds[i]->num_samples > greater_sound->num_samples)
            greater_sound = sounds[i];
    }
    wav_t* mix_sound = wav_copy(greater_sound);

    short* samples[n];
    for(int i = 0; i < n; i++){
        samples[i] = !sounds[i] ? NULL : sounds[i]->data;
    }

    int sum_max = 0;
    for(int i = 0; i < n; i++){
        if(!sounds[i] || !sounds[i]->data) continue;
        short max = abs(*samples[i]);
        for(int j = 1; j < sounds[i]->num_samples; j++){
            if(abs(samples[i][j]) > max) max = abs(samples[i][j]);
        }
        sum_max += max;
    }

    float factor = 1.0f;

    if(sum_max > MAX_AMPLITUDE_16BIT * 0.95f){
        factor = MAX_AMPLITUDE_16BIT*0.95f / (float)sum_max;
    }

    for(int i = 0; i < mix_sound->num_samples; i++){
        int sum = 0;
        for(int j = 0; j < n; j++){
            if(!sounds[j] || !sounds[j]->data) continue;
            if(i < sounds[j]->num_samples){
                sum += samples[j][i];
            }
        }
        ((short*)mix_sound->data)[i] = (short)(sum * factor);
    }

    return mix_sound;
}


wav_t* wav_synthesize(char* function, float t0, float time_duration, float freq, const char* file_name){
    unsigned rate = 44100;
    unsigned num_samples = time_duration * rate;
    wav_t* sound = wav_new(STD_HEADER(rate, 1, num_samples));
    int T = sound->header.sample_rate/freq;
    float yi, t_curr, sum = 0.0f;

    float* samples = malloc(T*sizeof(float));
    for(int i = 0; i < T; i++){
        t_curr = t0 + (float) i/ (float) sound->header.sample_rate;
        yi = eval_X(function, t_curr);
        samples[i] = isfinite(yi) ? yi : 0.0f;
        
        sum += samples[i];
    }

    float mean = sum / T, max = 0.00001;
    for(int i = 0; i < T; i++){
        samples[i] -= mean;
        if(fabsf(samples[i]) > max) max = fabsf(samples[i]);
    }

    for(int i = 0; i < num_samples; i++){
        ((short*)sound->data)[i] = (short) ((samples[i % T] / max)*(MAX_AMPLITUDE_16BIT * 0.95f)); 
    }

    if(file_name && *file_name) wav_write(sound, file_name);
    free(samples);
    return sound;
}

wav_t* wav_decay_exp(wav_t* sound, float k){
    float a = exp(-k / sound->header.sample_rate), curr = 1;
    wav_t* cp = wav_copy(sound);
    for(int i = 0; i < cp->num_frames; i++){
        for(int c = 0; c < cp->header.num_channels; c++){
            unsigned idx = i*cp->header.num_channels + c;
            ((short*)cp->data)[idx] = (((short*)cp->data)[idx]* curr);
        }
        curr *= a;
    }
    return cp;
}

wav_t* wav_decay_normal(wav_t* sound, float k){
    wav_t* cp = wav_copy(sound);
    for(int i = 0; i < cp->num_frames; i++){
        float t = (float)i / (float)sound->header.sample_rate;
        for(int c = 0; c < cp->header.num_channels; c++){
            unsigned idx = i*cp->header.num_channels + c;
            float factor = exp(-k*t*t);
            ((short*)cp->data)[idx] = (short) (((short*)cp->data)[idx] * factor);
        }
    }
    return cp;
}

wav_t* wav_tremble(wav_t* sound, float k){
    wav_t* cp = wav_copy(sound);
    for(int i = 0; i < cp->num_frames; i++){
        float t = (float)i / (float)sound->header.sample_rate;
        for(int c = 0; c < cp->header.num_channels; c++){
            unsigned idx = i*cp->header.num_channels + c;
            float factor = sinf(-k*t);
            ((short*)cp->data)[idx] = (short) (((short*)cp->data)[idx] * factor);
        }
    }
    return cp;
}

wav_t* wav_envelope(wav_t* sound, float k, float (*func)(float t, float k)){
    wav_t* cp = wav_copy(sound);

    float* samples_float = malloc(sound->num_samples * sizeof(float));
    float max = 0.00001;
    for(int i = 0; i < sound->num_frames; i++){
        float t = (float)i / (float)sound->header.sample_rate;
        for(int c = 0; c < sound->header.num_channels; c++){
            int idx = i*sound->header.num_channels + c;
            samples_float[idx] = ((short*) sound->data)[idx] * func(t, k);
            if(fabsf(samples_float[idx]) > max) max = fabsf(samples_float[idx]);
        }
    }

    float factor = 1.0f;
    if(max > 0.95f*MAX_AMPLITUDE_16BIT) factor = 0.95f * MAX_AMPLITUDE_16BIT/max;
    
    for(int i = 0; i < sound->num_samples; i++){
        ((short*)cp->data)[i] = (short) (samples_float[i] * factor);
    }
    
    free(samples_float);
    return cp;
}

wav_t* wav_to_mono(wav_t* sound){
    if(!sound || !sound->data) return NULL;
    if(sound->header.num_channels == 1) return wav_copy(sound);

    WavHeader h = STD_HEADER(sound->header.sample_rate, 1, sound->num_samples / sound->header.num_channels);
    wav_t* mono_sound = wav_new(h);
    short num_channels = sound->header.num_channels;

    for(int i = 0; i < mono_sound->num_samples; i++){
        int mean = 0;
        for(int c = 0; c < num_channels; c++){
            int idx = i * num_channels + c;
            mean += ((short*)sound->data)[idx];
        }
        mean /= num_channels;
        ((short*) mono_sound->data)[i] = (short) mean;
    }
    return mono_sound;
}

wav_t* wav_gaussian_blur(wav_t* sound, float sigma, int kernel_size){
    if(kernel_size % 2 == 0 || kernel_size <= 0 || !sound || sigma <= 0) return wav_copy(sound);
    float* kernel = malloc(sizeof(float) * kernel_size);
    int radius = kernel_size / 2;

    float sum_weavys = 0.0f;
    for(int i = -radius; i <= radius; i++){
        float dist = -(float)(i*i) / (2.0f * sigma * sigma);
        kernel[i + radius] = expf(dist);
        sum_weavys += kernel[i + radius];
    }

    for(int i = 0; i < kernel_size; i++){
        kernel[i] /= sum_weavys;
    }

    wav_t* blurred_sound = wav_copy(sound);
    short* samples_blurred = blurred_sound->data;
    float* samples_float = calloc(sound->num_samples, sizeof(float));

    short num_channels = sound->header.num_channels;
    for(int c = 0; c < num_channels; c++){
        for(int f = radius; f < sound->num_frames - radius; f++){
            for(int j = -radius; j <= radius; j++){
                samples_float[f*num_channels + c] += ((short*) sound->data)[(f + j)*num_channels + c] * kernel[j + radius];
            }
        }
    }
    
    for(int i = radius*num_channels; i < sound->num_samples - radius*num_channels; i++){
        samples_blurred[i] = (short)(samples_float[i]);
    }

    for(int i = 0; i < radius*num_channels; i++){
        samples_blurred[i] = ((short*)sound->data)[i];
        samples_blurred[sound->num_samples - i - 1] = ((short*)sound->data)[sound->num_samples - i - 1];
    }

    free(samples_float);
    free(kernel);
    return blurred_sound;
}
    
wav_t* wav_bandpass_filter(wav_t* sound, float fA, float fB){
    if(!sound) return NULL;
    float fs = (float) sound->header.sample_rate;
    float f0 = (fA + fB) / 2.0f;
    float fc = fA;
    float sigma_f = fabsf(fc - f0) / sqrt(log(2));
    float sigma_t = 1.0f/(2.0f * M_PI * sigma_f);
    float sigma_n =  sigma_t * fs; // sqrt(ln2) / 2pi
    int kernel_size = 2 * ceilf(3*sigma_n) + 1;

    float *kernel = malloc(kernel_size * sizeof(float));
    int radius = kernel_size / 2;

    for(int i = -radius; i <= radius; i++){
        kernel[i + radius] = sigma_f*sqrt(2*M_PI) * expf(-i*i/(2*sigma_n*sigma_n)) * cosf(2*M_PI*f0*i / fs);
    }
    
    float* samples_float = calloc(sound->num_samples, sizeof(float));

    wav_t* filtered_sound = wav_copy(sound);
    short* samples = filtered_sound->data;
    short num_channels = sound->header.num_channels;
    for(int c = 0; c < num_channels; c++){
        for(int f = radius; f < sound->num_frames - radius; f++){
            for(int k = -radius; k <= radius; k++){
                
                samples_float[f*num_channels + c] += samples[(f + k)*num_channels + c] * kernel[radius + k];
            }
        }
    }

    for(int i = 0; i < sound->num_samples; i++){
        float filtered_sample = samples_float[i];
        if(filtered_sample > MAX_AMPLITUDE_16BIT) filtered_sample = 0.95f * MAX_AMPLITUDE_16BIT;
        if(filtered_sample < MIN_AMPLITUDE_16BIT) filtered_sample = 0.95f * MIN_AMPLITUDE_16BIT;

        samples[i] = (short)filtered_sample;
    } 

    free(kernel);
    free(samples_float);
    return filtered_sound;
}

wav_t* wav_band_reject_filter(wav_t* sound, float fA, float fB){
    wav_t* filtered = wav_bandpass_filter(sound, fA, fB);
    short *samples = sound->data, *filtered_samples = filtered->data;
    int max_dif = 0;
    for(int i = 0; i < sound->num_samples; i++){
        int dif = (samples[i] - filtered_samples[i]);
        if(abs(dif) > max_dif) max_dif = abs(dif);
    }
    float factor = 1.0f;
    if(max_dif > MAX_AMPLITUDE_16BIT && max_dif > 0) factor = 0.95f*MAX_AMPLITUDE_16BIT / (float)max_dif; 

    for(int i = 0; i < sound->num_samples; i++){
        int dif = (samples[i] - filtered_samples[i]);
        filtered_samples[i] = (short)(dif*factor);
    }

    return filtered;
}

wav_t* wav_lowpass_filter(wav_t* sound, float fc){
    return wav_bandpass_filter(sound, 0, fc);
}

wav_t* wav_highpass_filter(wav_t* sound, float fc){
    return wav_band_reject_filter(sound, 0, fc);
}

wav_t* wav_resample(wav_t* sound, unsigned new_sample_rate){
    if(new_sample_rate >= sound->header.sample_rate) return wav_copy(sound);

    float p = (float) sound->header.sample_rate / (float) new_sample_rate;
    
    short num_channels = sound->header.num_channels;
    unsigned new_num_samples = (unsigned)((float)sound->num_samples / p);
    wav_t* new = wav_new(STD_HEADER(new_sample_rate, num_channels, new_num_samples));
    size_t size = new->header.block_align;
    for(int i = 0; i < new->num_frames; i++){
        int idx = i*p;
        if(idx >= sound->num_frames){
            idx = sound->num_frames - 1;
        }
        memcpy(new->data + i*size, sound->data + idx*size, size);
    }

    return new;
}

wav_t* wav_adjust_velocity(wav_t* sound, float p){

}

int main(){

    wav_t* wv = wav_read("tracks/vaqueiro_nao.wav");
    unsigned new_fc = 15000; 
    wav_t* wv_high = wav_highpass_filter(wv, new_fc);

    wav_write(wv_high, "tracks/vaqueiro_nao_high.wav");
    return 0;
}
