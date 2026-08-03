#include "WavLib.h"


int main(){
    wav_t* wv = wav_read("tracks/chama.wav");
    if(wv) printf("abriu");
    
    wav_gaussian_bandpass(wv, 220, 440, true);
    wav_delete(&wv);
    return 0;
}
