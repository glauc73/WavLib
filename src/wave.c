#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    unsigned subchunk2_size;
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
    .subchunk2_size = (num_samples) * (channels) * sizeof(short) \
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
    sound->header = h;
    sound->data = malloc(h.subchunk2_size);
    sound->num_samples = sound->header.subchunk2_size/(sound->header.block_align);
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

    sound->num_samples = sound->header.subchunk2_size/(sound->header.block_align);
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
    if(max * k > MAX_AMPLITUDE_16BIT*0.8f)
        factor = (MAX_AMPLITUDE_16BIT*0.8f) / max;

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
    
    short max1 = abs(samples1[0]), max2 = abs(samples2[0]);
    for(int i = 1; i < sound1->num_samples; i++)
        if(abs(samples1[i]) > max1) max1 = abs(samples1[i]);
    for(int i = 0; i < sound2->num_samples; i++)
        if(abs(samples2[i]) > max2) max2 = abs(samples2[i]);
        
    float factor = 1.0f;
    if(max1 + max2 > MAX_AMPLITUDE_16BIT*0.8f)
        factor = (MAX_AMPLITUDE_16BIT*0.8f) / (max1 + max2);

    for(int i = 0; i < less_sound->num_samples; i++){
        int mixed_int = (int)((samples1[i] + samples2[i]) * factor);
        ((short*)mix_sound->data)[i] = (short)mixed_int;
    }

    for(int i = less_sound->num_samples; i < greater_sound->num_samples; i++){
        ((short*)mix_sound->data)[i] *= factor;
    }

    return mix_sound;
}


wav_t* wav_mix_array(wav_t** sounds, int n){
    if (!sounds) return NULL;
    for(int i = 0; i < n; i++){
        if (sounds[i]->header.sample_rate != sounds[i + 1]->header.sample_rate) {
            printf("Erro [wav_mix]: Taxas de amostragem incompatíveis (%u Hz vs %u Hz).\n", 
                sounds[i]->header.sample_rate, sounds[i + 1]->header.sample_rate);
            return NULL;
        }
        if (sounds[i]->header.num_channels != sounds[i + 1]->header.num_channels) {
            printf("Erro [wav_mix]: Número de canais incompatível (%d ch vs %d ch).\n", 
                sounds[i]->header.num_channels, sounds[i + 1]->header.num_channels);
            return NULL;
        }
        if (sounds[i]->header.bits_per_sample != sounds[i + 1]->header.bits_per_sample) {
            printf("Erro [wav_mix]: bits por amostra incompatíveis (%d bits vs %d bits).\n", 
                sounds[i]->header.bits_per_sample, sounds[i + 1]->header.bits_per_sample);
            return NULL;
        }
    }
    wav_t* greater_sound = sound1->num_samples > sound2->num_samples ? sound1 : sound2;
    wav_t* less_sound = sound1->num_samples < sound2->num_samples ? sound1 : sound2;
    wav_t* mix_sound = wav_copy(greater_sound);

    short* samples1 = sound1->data, *samples2 = sound2->data;
    
    short max1 = abs(samples1[0]), max2 = abs(samples2[0]);
    for(int i = 1; i < sound1->num_samples; i++)
        if(abs(samples1[i]) > max1) max1 = abs(samples1[i]);
    for(int i = 0; i < sound2->num_samples; i++)
        if(abs(samples2[i]) > max2) max2 = abs(samples2[i]);
        
    float factor = 1.0f;
    if(max1 + max2 > MAX_AMPLITUDE_16BIT*0.8f)
        factor = (MAX_AMPLITUDE_16BIT*0.8f) / (max1 + max2);

    for(int i = 0; i < less_sound->num_samples; i++){
        int mixed_int = (int)((samples1[i] + samples2[i]) * factor);
        ((short*)mix_sound->data)[i] = (short)mixed_int;
    }

    for(int i = less_sound->num_samples; i < greater_sound->num_samples; i++){
        ((short*)mix_sound->data)[i] *= factor;
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
        ((short*)sound->data)[i] = (short) ((samples[i % T] / max)*(MAX_AMPLITUDE_16BIT * 0.8f)); 
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

wav_t* wav_to_mono(wav_t* sound){

}

wav_t* wav_gaussian_blur(wav_t* sound, float sigma, int kernel_size){

}

wav_t* wav_decrease_sample_rate(wav_t* sound, unsigned new_sample_rate){

}

wav_t* wav_adjust_velocity(wav_t* sound, float p){

}

int main(){
    char func[128], file_name[64];
    float freq;
    fgets(func, sizeof(func), stdin);
    scanf("%f", &freq);
    getchar();
    fgets(file_name, sizeof(file_name), stdin);

    func[strcspn(func, "\n")] = '\0';
    file_name[strcspn(file_name, "\n")] = '\0';
    
    wav_t* wv = wav_synthesize(func, 0, 2, freq, file_name);
    wv = wav_decay_normal(wv, 5.0f);
    wav_write(wv, "tracks/chama.wav");
    return 0;
}
