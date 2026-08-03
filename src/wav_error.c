#include "wav_error.h"
#include <stdio.h>
#include <stdarg.h>


WavError_t wav_error = {WAV_SUCCESS, ""};

void wav_set_error(wav_error_code_t code, const char* msg, ...){
    if(!msg) return;
    va_list args;
    va_start(args, msg);

    wav_error.code = code;
    vsnprintf(wav_error.message, sizeof(wav_error.message), msg, args);
    va_end(args);
}
