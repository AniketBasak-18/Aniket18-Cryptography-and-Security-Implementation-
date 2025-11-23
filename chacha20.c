#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define ROTL(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
#define QR(a,b,c,d) \
    a += b; d ^= a; d = ROTL(d,16); \
    c += d; b ^= c; b = ROTL(b,12); \
    a += b; d ^= a; d = ROTL(d,8);  \
    c += d; b ^= c; b = ROTL(b,7);

void chacha20_block(uint32_t out[16], const uint32_t in[16]) 
{
    int i;
    uint32_t x[16];
    memcpy(x, in, sizeof(x));
    for (i = 0; i < 10; i++) 
    {
        QR(x[0], x[4], x[8], x[12]);
        QR(x[1], x[5], x[9], x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8], x[13]);
        QR(x[3], x[4], x[9], x[14]);
    }
    for (i = 0; i < 16; i++)
        out[i] = x[i] + in[i];
}

static uint32_t load32_le(const uint8_t *src) 
{
    return ((uint32_t)src[0]) | ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static void store32_le(uint8_t *dst, uint32_t w) 
{
    dst[0] = w & 0xff;
    dst[1] = (w >> 8) & 0xff;
    dst[2] = (w >> 16) & 0xff;
    dst[3] = (w >> 24) & 0xff;
}

void chacha20_encrypt(
    uint8_t *out, const uint8_t *in, size_t len,
    const uint8_t key[32], const uint8_t nonce[12],
    uint32_t counter
) 
{
    uint32_t state[16], block[16];
    uint8_t keystream[64];
    size_t i, j;

    state[0] = 0x61707865; state[1] = 0x3320646e;
    state[2] = 0x79622d32; state[3] = 0x6b206574;

    for (i = 0; i < 8; i++)
        state[4 + i] = load32_le(key + i * 4);

    state[12] = counter;
    for (i = 0; i < 3; i++)
        state[13 + i] = load32_le(nonce + i * 4);

    for (i = 0; i < len; i += 64) 
    {
        chacha20_block(block, state);
        for (j = 0; j < 16; j++)
            store32_le(keystream + 4 * j, block[j]);

        size_t block_len = (len - i < 64) ? len - i : 64;
        for (j = 0; j < block_len; j++)
            out[i + j] = in[i + j] ^ keystream[j];

        state[12]++;
    }
}

int main() 
{
    uint8_t key[32] = {0};   // All-zero 256-bit key
    uint8_t nonce[12] = {0}; // All-zero 96-bit nonce
    char input[1024];
    uint8_t output[1024];
    int mode;

    printf("ChaCha20 Cipher\n");
    printf("1. Encrypt\n2. Decrypt\nChoose mode (1 or 2): ");
    scanf("%d", &mode);
    getchar(); // consume newline

    printf("Enter your message: ");
    fgets(input, sizeof(input), stdin);
    size_t len = strlen(input);
    if (input[len - 1] == '\n') input[--len] = '\0';

    chacha20_encrypt(output, (uint8_t *)input, len, key, nonce, 0);

    if (mode == 1) 
    {
        printf("\nCiphertext (hex): ");
        for (size_t i = 0; i < len; i++)
            printf("%02x", output[i]);
        printf("\n");
    } else 
    {
        printf("\nDecrypted text: ");
        for (size_t i = 0; i < len; i++)
            printf("%c", output[i]);
        printf("\n");
    }

    return 0;
}
