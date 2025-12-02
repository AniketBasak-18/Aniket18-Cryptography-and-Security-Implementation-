#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <x86intrin.h>  // For __rdtsc()

#define N 256   // 2^8

void swap(unsigned char *a, unsigned char *b) 
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int KSA(char *key, unsigned char *S) 
{
    int len = strlen(key);
    int j = 0;

    for(int i = 0; i < N; i++)
        S[i] = i;

    for(int i = 0; i < N; i++) 
    {
        j = (j + S[i] + key[i % len]) % N;
        swap(&S[i], &S[j]);
    }

    return 0;
}

int PRGA(unsigned char *S, char *plaintext, unsigned char *ciphertext) 
{
    int i = 0;
    int j = 0;

    for(size_t n = 0, len = strlen(plaintext); n < len; n++) 
    {
        i = (i + 1) % N;
        j = (j + S[i]) % N;

        swap(&S[i], &S[j]);
        int rnd = S[(S[i] + S[j]) % N];

        ciphertext[n] = rnd ^ plaintext[n];
    }

    return 0;
}

int RC4(char *key, char *plaintext, unsigned char *ciphertext) 
{
    unsigned char S[N];
    KSA(key, S);
    PRGA(S, plaintext, ciphertext);
    return 0;
}

int main(int argc, char *argv[]) 
{
    if(argc < 3) 
    {
        printf("Usage: %s <key> <plaintext>\n", argv[0]);
        return -1;
    }

    size_t len = strlen(argv[2]);
    unsigned char *ciphertext = malloc(len);

    // Read TSC before RC4
    unsigned long long start = __rdtsc();

    RC4(argv[1], argv[2], ciphertext);

    // Read TSC after RC4
    unsigned long long end = __rdtsc();

    printf("Ciphertext (hex): ");
    for(size_t i = 0; i < len; i++)
        printf("%02X ", ciphertext[i]);
    printf("\n");

    printf("CPU Clock Cycles: %llu\n", end - start);

    free(ciphertext);
    return 0;
}
