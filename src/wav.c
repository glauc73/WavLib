#include "wav.h"

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
    const char* ext = strrchr(file_name, '.');
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

void wav_write(const wav_t* sound, const char* file_name){
    if(sound == NULL) return;
    const char* ext = strrchr(file_name, '.');
    if(!ext || strcmp(ext, ".wav") != 0){
        printf("Só é possível manipular arquivos com extensão .wav\n");
        exit(EXIT_FAILURE);
    }
    FILE* f = fopen(file_name, "wb");

    fwrite(&(sound->header), sizeof(WavHeader), 1, f);
    fwrite(sound->data, 1, sound->header.subchunk2_size, f);

    fclose(f);
}

wav_t* wav_copy(const wav_t* sound){
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

wav_t* wav_to_mono(wav_t* sound, bool inplace){
    if(!sound || !sound->data) return NULL;
    if(sound->header.num_channels == 1) return inplace ? sound : wav_copy(sound);
    
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) return NULL;

    short* samples = target->data;
    short num_channels = target->header.num_channels;
    for(int i = 0; i < target->num_frames; i++){
        float mean = 0;
        for(int c = 0; c < num_channels; c++){
            mean += samples[i*num_channels + c];
        }
        mean /= num_channels;
        samples[i] = (short)roundf(mean);
    }
    unsigned num_samples = target->num_frames;
    target->header = STD_HEADER(target->header.sample_rate, 1, num_samples);
    target->num_frames = num_samples;
    target->num_samples = num_samples;

    target->data = realloc(samples, target->header.subchunk2_size);

    return target;
}

wav_t* wav_resample(wav_t* sound, unsigned new_sample_rate, bool inplace){
    if(new_sample_rate >= sound->header.sample_rate) return NULL;
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) return NULL;

    float p = (float) sound->header.sample_rate / (float) new_sample_rate;

    short num_channels = target->header.num_channels;
    unsigned new_num_samples = (unsigned)((float)target->num_samples / p);
    unsigned new_num_frames = (unsigned)((float)target->num_frames / p);

    size_t size = target->header.block_align;
    for(int i = 0; i < new_num_frames; i++){
        int idx = i*p;
        if(idx >= target->num_frames){
            idx = target->num_frames - 1;
        }
        memcpy((char*)target->data + i*size, (char*)target->data + idx*size, size);
    }

    target->header = STD_HEADER(new_sample_rate, num_channels, new_num_samples);
    target->num_samples = new_num_samples;
    target->num_frames = new_num_frames;

    target->data = realloc(target->data, target->header.subchunk2_size);
    return target;
}
