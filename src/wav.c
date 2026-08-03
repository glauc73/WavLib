#include "wav.h"
#include "wav_error.h"
#include <stddef.h>
#include <stdint.h>

wav_t* wav_new(WavHeader h){
    wav_t* sound = malloc(sizeof(wav_t));
    if(!sound) {
        wav_set_error(WAV_ERR_OUT_OF_MEMORY, "ERROR: Out of Memory!\n"); 
        return NULL;
    }
    sound->header = h;
    sound->data = malloc(h.subchunk2_size);

    if(!sound->data) {
        wav_set_error(WAV_ERR_OUT_OF_MEMORY, "ERROR: Out of Memory!\n"); 
        return NULL;
    }

    sound->num_samples = sound->header.subchunk2_size/(sound->header.bits_per_sample / 8);
    sound->num_frames = sound->num_samples / sound->header.num_channels;
    sound->time_duration = (float)sound->num_frames / (float)sound->header.sample_rate;
    return sound;
}


wav_t* wav_read(const char* file_name){
    const char* ext = strrchr(file_name, '.');
    if(!ext || strcmp(ext, ".wav") != 0){
        wav_set_error(
            WAV_ERR_INVALID_FORMAT,
            "ERRO: Só é possível manipular arquivos com extensão .wav\n"
        );
        return NULL;
    }
    FILE* f = fopen(file_name, "rb");
    if(f == NULL){  
        wav_set_error(
            WAV_ERR_FILE_NOT_FOUND,
            "ERRO: O arquivo '%s' não existe no diretório atual\n", file_name
        );
        return NULL;
    }

    wav_t* sound = malloc(sizeof(wav_t));

    fread(&(sound->header), sizeof(WavHeader), 1, f);

    if(sound->header.bits_per_sample != 16){
        wav_set_error(
            WAV_ERR_INCOMPATIBLE_HEADER,
            "ERRO: O arquivo '%s' não é um áudio PCM de 16 bit\n", file_name
        );
        fclose(f);
        wav_delete(&sound);
        return NULL;
    }

    sound->data = malloc(sound->header.subchunk2_size);
    
    fread(sound->data, 1, sound->header.subchunk2_size, f);
    fclose(f);

    sound->num_samples = sound->header.subchunk2_size/(sound->header.bits_per_sample / 8);
    sound->num_frames = sound->num_samples / sound->header.num_channels;
    sound->time_duration = (float)sound->num_frames / (float)sound->header.sample_rate;
   
    wav_set_error(WAV_SUCCESS, "");
    return sound;
}

void wav_write(const wav_t* sound, const char* file_name){
    if(sound == NULL) return;
    const char* ext = strrchr(file_name, '.');
    if(!ext || strcmp(ext, ".wav") != 0){
        wav_set_error(
            WAV_ERR_INVALID_FORMAT,
            "ERRO: Só é possível manipular arquivos com extensão .wav\n"
        );
        return;
    }
    FILE* f = fopen(file_name, "wb");

    fwrite(&(sound->header), sizeof(WavHeader), 1, f);
    fwrite(sound->data, 1, sound->header.subchunk2_size, f);

    fclose(f);
}

wav_t* wav_copy(const wav_t* sound){
    if(!sound){
        wav_set_error(WAV_ERR_NULL_POINTER,  "ERROR: %s is a NULL pointer (%s)", tostr(sound), __FUNCTION__);
    }
    wav_t* cp = malloc(sizeof(wav_t));
    if(!cp) {
        wav_set_error(WAV_ERR_OUT_OF_MEMORY, "ERROR: Out of Memory\n");
        return NULL;
    }

    *cp = *sound;
    cp->data = malloc(cp->header.subchunk2_size);
    if(!cp->data) {
        wav_set_error(WAV_ERR_OUT_OF_MEMORY, "ERROR: Out of Memory\n");
        free(cp); 
        return NULL;
    }

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
    if(!sound || !sound->data) {
        wav_set_error(
            WAV_ERR_NULL_POINTER, 
            "ERROR [%s]: '%s' is a NULL pointer\n", __FUNCTION__, tostr(sound)
        );
        return NULL;
    }
    if(sound->header.num_channels == 1) return inplace ? sound : wav_copy(sound);
    
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target) return NULL;

    short num_channels = target->header.num_channels;

    for(int i = 0; i < target->num_frames; i++){
        float mean = 0;
        float val = 0.0f;
        for(int c = 0; c < num_channels; c++){
            mean += target->data[i*num_channels + c];
        }
        mean /= num_channels;
        target->data[i] = mean;
    }
    unsigned num_samples = target->num_frames;
    target->header = STD_HEADER(target->header.sample_rate, 1, num_samples);
    target->num_frames = num_samples;
    target->num_samples = num_samples;

    target->data = realloc(target->data, target->header.subchunk2_size);

    return target;
}

wav_t* wav_resample(wav_t* sound, unsigned new_sample_rate, bool inplace){
    if(new_sample_rate >= sound->header.sample_rate) return NULL;
    wav_t* target = inplace ? sound : wav_copy(sound);
    if(!target){
        wav_set_error(WAV_ERR_OUT_OF_MEMORY, "ERROR: Out of Memory\n");
    }

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
