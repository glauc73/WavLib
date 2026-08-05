#include "fft.h"
#include <stddef.h>
#include <stdint.h>

int main(){

    float A[] = {1, 2};
    float B[] = {3, -1};

    uint64_t nA = 2;
    uint64_t nB = 2;
    uint64_t n = nA + nB - 1;

    fft_t* ft_A = fft_init_from_time(A, nA, n);
    fft_t* ft_B = fft_init_from_time(B, nA, n);
    
    fft_run(ft_A);
    fft_run(ft_B);

    fft_t* ft_C = fft_new(n, FREQUENCY);
    
    for(int i = 0; i < ft_C->N; i++){
        ft_C->data[i] = ft_A->data[i] * ft_B->data[i];
    }

    ifft_run(ft_C);

    for(int i = 0; i < ft_C->N; i++){
        printf("%d : %f\n", i, creal(ft_C->data[i]));
    }

    printf("domain : %s\n", ft_C->domain == TIME ? "time" : "frequency");

    fft_delete(&ft_A);
    fft_delete(&ft_B);
    fft_delete(&ft_C);
    return 0;
}
