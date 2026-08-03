#include "wav_error.h"
#include "wavlib.h"

int main(){
    wav_t* wv = wav_read("tracks/chama");
    if(!wav_error.code) printf("abriu"); 
    else{
        printf(wav_error.message);
    }
    wav_gaussian_bandpass(wv, 220, 440, true);
    wav_delete(&wv);
    return 0;
}
