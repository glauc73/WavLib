
#include "fft.h"
#include "wav.h"
#include "wav_filters.h"
#include "wavlib.h"
#include <stddef.h>
#include <stdint.h>

int main(){
    wav_t* eco = wav_read("tracks/eco.wav");
    wav_t* sound = wav_read("tracks/vaqueiro_nao.wav");
   
    wav_to_mono(sound, true);
    wav_to_mono(eco, true);

    wav_gaussian_lowpass(eco, sound->header.sample_rate/2.0f, true);
    wav_resample(eco, sound->header.sample_rate, true); 

    size_t N = eco->num_samples + sound->num_samples - 1;
    
    float* samples_float_eco = malloc(eco->num_samples * sizeof(float));
    float* samples_float_sound = malloc(sound->num_samples * sizeof(float));

    for(int i = 0; i < eco->num_samples; i++) samples_float_eco[i] = (float)eco->data[i] / MAX_AMPLITUDE_16BIT;
    for(int i = 0; i < sound->num_samples; i++) samples_float_sound[i] = (float) sound->data[i] / MAX_AMPLITUDE_16BIT;
        
    fft_t* ft_eco = fft_init_from_time(samples_float_eco, eco->num_samples, N);
    fft_t* ft_sound = fft_init_from_time(samples_float_sound, sound->num_samples, N);

    fft_run(ft_eco);
    fft_run(ft_sound);

    fft_t* conv = fft_new(N, FREQUENCY);

    for(int i = 0; i < conv->N; i++)
        conv->data[i] = ft_eco->data[i] * ft_sound->data[i];

    ifft_run(conv);    
    fft_norm_max(conv);

    wav_t* wv = wav_new(STD_HEADER(sound->header.sample_rate, 1, ft_sound->n_samples));

    for(int i = 0; i < ft_sound->n_samples; i++){
        int sample = roundf(crealf(conv->data[i])*MAX_AMPLITUDE_16BIT);
        if(sample > MAX_AMPLITUDE_16BIT) sample = MAX_AMPLITUDE_16BIT;
        if(sample < MIN_AMPLITUDE_16BIT) sample = MIN_AMPLITUDE_16BIT;
        wv->data[i] = sample;
    }

    wav_write(wv, "tracks/eco_resampled.wav");

    free(samples_float_sound);
    free(samples_float_eco);
    fft_delete(&ft_eco);
    fft_delete(&ft_sound);
    fft_delete(&conv);
    wav_delete(&eco);
    wav_delete(&wv);
    wav_delete(&sound);
    
    return 0;
}
