#ifndef _WAV_ERRORS_H_
#define _WAV_ERRORS_H_

typedef enum{
    WAV_SUCCESS = 0,
    WAV_ERR_FILE_NOT_FOUND,
    WAV_ERR_INVALID_FORMAT,
    WAV_ERR_NULL_POINTER,
    WAV_ERR_OUT_OF_MEMORY,
    WAV_ERR_INCOMPATIBLE_HEADER,
}wav_error_code_t;

typedef struct{
    wav_error_code_t code;
    char message[256];
}WavError_t;

void wav_set_error(wav_error_code_t code, const char* msg, ...);

extern WavError_t wav_error;
#endif
