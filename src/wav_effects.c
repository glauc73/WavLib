#include "wav.h"
#include "wav_effects.h"
#include "fourier.h"

void wav_adjust_volume(wav_t* sound, float dB){
    if(!sound) return;

    float k = powf(10.0f, dB/20.0f);
    float max = 0.0f;

    for(int i = 1; i < sound->num_samples; i++){
        float abs_val = abs(sound->data[i]);
        if(abs_val > max) max = abs_val;
    }
    if(max == 0.0f) return;

    float factor = k;
    if(max * k > MAX_AMPLITUDE_16BIT)
        factor = MAX_AMPLITUDE_16BIT / max;

    for(int i = 0; i < sound->num_samples; i++){
        sound->data[i] = (short)(sound->data[i] * factor);
    }
}

wav_t* wav_mix(wav_t* sound1, wav_t* sound2){
    wav_t* mix[2] = {sound1, sound2};
    return wav_mix_array(mix, 2);
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

wav_t* wav_synthesize(
    char* function, 
    float t0,
    float T,
    float time_duration, 
    float freq, 
    unsigned sample_rate,
    int n_harmonics,
    const char* file_name
){
    
    unsigned num_samples = roundf(time_duration * sample_rate);
    wav_t* wv = wav_new(STD_HEADER(sample_rate, 1, num_samples));
    FourierSeriesCoefs* four = ComputeFourierSeriesCoefs(function, t0, T, n_harmonics);   
   
    float* samples = malloc(wv->num_samples * sizeof(float));
    float max = 0.0f, sum = 0.0f; 
    for(int i = 0; i < wv->num_samples; i++){
        float t_curr = t0 + (float)i/sample_rate;
        samples[i] = FourierSeries(four, t_curr, freq); 
        sum += samples[i];
    }

    float mean = sum/wv->num_samples;

    for(int i = 0; i < wv->num_samples; i++){
        samples[i] -= mean;
        if(fabsf(samples[i]) > max) max = fabsf(samples[i]);
    }

    float factor = 1.0f;
    if(max > 0.0f) factor = 0.95f * MAX_AMPLITUDE_16BIT / max;

    for(int i = 0; i < wv->num_samples; i++){
        ((short*)wv->data)[i] = (short)(samples[i] * factor);
    }

    if(file_name && *file_name) wav_write(wv, file_name);

    free(four->an), free(four->bn), free(four);
    free(samples);
    return wv;
}

wav_t* wav_decay_exp(wav_t* sound, float k, bool inplace){
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) return NULL;

    float a = exp(-k / target->header.sample_rate), curr = 1;
    short* samples = target->data;
    for(int i = 0; i < target->num_frames; i++){
        for(int c = 0; c < target->header.num_channels; c++){
            unsigned idx = i*target->header.num_channels + c;
            samples[idx] = samples[idx]* curr;
        }
        curr *= a;
    }
    return target;
}

wav_t* wav_gaussian_window(wav_t* sound, float sigma, float mu, bool inplace){
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) return NULL;

    short* samples = target->data;
    for(int i = 0; i < target->num_frames; i++){
        float t = (float)i / (float)target->header.sample_rate;
        for(int c = 0; c < target->header.num_channels; c++){
            unsigned idx = i*target->header.num_channels + c;
            float factor = exp(-(t - mu)*(t - mu) / (2*(sigma * sigma)));
            samples[idx] = (short)(samples[idx] * factor);
        }
    }   
    return target;
}

wav_t* wav_tremble(wav_t* sound, float k, bool inplace){
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) return NULL;

    for(int i = 0; i < target->num_frames; i++){
        float t = (float)i / (float)sound->header.sample_rate;
        for(int c = 0; c < target->header.num_channels; c++){
            unsigned idx = i*target->header.num_channels + c;
            float factor = sinf(-k*t);
            ((short*)target->data)[idx] = (short) (((short*)target->data)[idx] * factor);
        }
    }
    return target;
}


