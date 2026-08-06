#include "wav_filters.h"
#include "fft.h"
#include "wav.h"

wav_t* wav_gaussian_bandpass(wav_t* sound, float fA, float fB, bool inplace){
    if(!sound) return NULL;
    float fs = (float) sound->header.sample_rate;
    float f0 = 0.0f;
    if(fA != 0.0f) f0 = (fA + fB) / 2.0f;
    float fc = fB;

    float sigma_f = fabsf(fc - f0) / sqrt(log(2));
    float sigma_t = 1.0f/(2.0f * M_PI * sigma_f);
    float sigma_n =  sigma_t * fs; // sqrt(ln2) / 2pi
    int kernel_size = 2 * ceilf(3*sigma_n) + 1;

    float *kernel = malloc(kernel_size * sizeof(float));
    if(!kernel) return NULL;

    int radius = kernel_size / 2;

    float complex H_f0_ = 0;
    for(int i = -radius; i <= radius; i++){
        kernel[i + radius] = sigma_f*sqrt(2*M_PI) * expf(-i*i/(2*sigma_n*sigma_n)) * cosf(2*M_PI*f0*i / fs);
         H_f0_ += kernel[i + radius] * cexpf(-2*M_PI*I*f0*i / fs);
    }

    float norm = cabs(H_f0_);
    for(int i = 0; i < kernel_size; i++) kernel[i] /= norm;

    float* samples_float = calloc(sound->num_samples, sizeof(float));
    if(!samples_float) {free(kernel); return NULL;}

    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) {
        free(samples_float); free(kernel);
        return NULL;
    }

    short num_channels = target->header.num_channels;
    for(int c = 0; c < num_channels; c++){
        for(int f = radius; f < target->num_frames - radius; f++){
            for(int k = -radius; k <= radius; k++){ 
                samples_float[f*num_channels + c] += target->data[(f + k)*num_channels + c] * kernel[radius + k];
            }
        }
    }

    for(int i = 0; i < sound->num_samples; i++){
        float filtered_sample = samples_float[i];
        if(filtered_sample > MAX_AMPLITUDE_16BIT) filtered_sample = MAX_AMPLITUDE_16BIT;
        if(filtered_sample < MIN_AMPLITUDE_16BIT) filtered_sample = MIN_AMPLITUDE_16BIT;

        target->data[i] = (short)filtered_sample;
    } 

    free(kernel);
    free(samples_float);

    return target;
}

wav_t* wav_gaussian_bandreject(wav_t* sound, float fA, float fB, bool inplace){
    wav_t* filtered = wav_gaussian_bandpass(sound, fA, fB, false);

    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) {
        wav_delete(&filtered); 
        return NULL;
    }

    short *filtered_samples = filtered->data;
    int max_dif = 0;
    for(int i = 0; i < sound->num_samples; i++){
        int dif = (target->data[i] - filtered_samples[i]);
        if(abs(dif) > max_dif) max_dif = abs(dif);
    }
    float factor = 1.0f;
    if(max_dif > MAX_AMPLITUDE_16BIT && max_dif > 0) factor = 0.95f*MAX_AMPLITUDE_16BIT / (float)max_dif; 

    for(int i = 0; i < sound->num_samples; i++){
        int dif = (target->data[i] - filtered_samples[i]);
        target->data[i] = (short)(dif*factor);
    }
    wav_delete(&filtered);
    return target;
}

wav_t* wav_gaussian_lowpass(wav_t* sound, float fc, bool inplace){
    return wav_gaussian_bandpass(sound, 0, fc, inplace);
}

wav_t* wav_gaussian_highpass(wav_t* sound, float fc, bool inplace){
    return wav_gaussian_bandreject(sound, 0, fc, inplace);
}

wav_t* wav_covolution(wav_t* snd1, wav_t* snd2){ 


}
