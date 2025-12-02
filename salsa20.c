#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define ROUNDS 20  // 20 rounds is standard for Salsa20

#define ROTL(a,b) (((a) << (b)) | ((a) >> (32 - (b))))

// Quarter-round function
#define QR(a,b,c,d) \
    b ^= ROTL(a + d, 7); \
    c ^= ROTL(b + a, 9); \
    d ^= ROTL(c + b,13); \
    a ^= ROTL(d + c,18);

// Salsa20 core function
void salsa20_block(uint32_t out[16], const uint32_t in[16]) {
    int i;
    uint32_t x[16];
    memcpy(x, in, sizeof(x));

    for (i = 0; i < ROUNDS; i += 2) {
        // Column rounds
        QR(x[0], x[4], x[8], x[12]);
        QR(x[5], x[9], x[13], x[1]);
        QR(x[10], x[14], x[2], x[6]);
        QR(x[15], x[3], x[7], x[11]);

        // Row rounds
        QR(x[0], x[1], x[2], x[3]);
        QR(x[5], x[6], x[7], x[4]);
        QR(x[10], x[11], x[8], x[9]);
        QR(x[15], x[12], x[13], x[14]);
    }

    for (i = 0; i < 16; ++i)
        out[i] = x[i] + in[i];
}

// Helper to convert bytes to 32-bit little-endian
uint32_t U8TO32_LE(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Fill input state (16x 32-bit words) using key, nonce, counter
void salsa20_keysetup(uint32_t input[16], const uint8_t key[32], const uint8_t nonce[8], uint64_t counter) {
    const char *constants = "expand 32-byte k";

    input[0] = U8TO32_LE((const uint8_t *)(constants + 0));
    input[1] = U8TO32_LE(key + 0);
    input[2] = U8TO32_LE(key + 4);
    input[3] = U8TO32_LE(key + 8);
    input[4] = U8TO32_LE(key + 12);
    input[5] = U8TO32_LE((const uint8_t *)(constants + 4));
    input[6] = U8TO32_LE(nonce + 0);
    input[7] = U8TO32_LE(nonce + 4);
    input[8] = (uint32_t)(counter & 0xFFFFFFFF);
    input[9] = (uint32_t)(counter >> 32);
    input[10] = U8TO32_LE((const uint8_t *)(constants + 8));
    input[11] = U8TO32_LE(key + 16);
    input[12] = U8TO32_LE(key + 20);
    input[13] = U8TO32_LE(key + 24);
    input[14] = U8TO32_LE(key + 28);
    input[15] = U8TO32_LE((const uint8_t *)(constants + 12));
}

void salsa20_encrypt(const uint8_t *key, const uint8_t *nonce, const uint8_t *in, uint8_t *out, size_t len) {
    uint32_t input[16], keystream[16];
    uint8_t block[64];
    uint64_t counter = 0;
    size_t i, j;

    while (len > 0) {
        salsa20_keysetup(input, key, nonce, counter);
        salsa20_block(keystream, input);

        for (i = 0; i < 16; ++i) {
            block[4*i + 0] = keystream[i] & 0xff;
            block[4*i + 1] = (keystream[i] >> 8) & 0xff;
            block[4*i + 2] = (keystream[i] >> 16) & 0xff;
            block[4*i + 3] = (keystream[i] >> 24) & 0xff;
        }

        size_t block_size = (len < 64) ? len : 64;

        for (j = 0; j < block_size; ++j)
            out[j] = in[j] ^ block[j];

        len -= block_size;
        in += block_size;
        out += block_size;
        counter++;
    }
}

// ---- MAIN FOR DEMO ----
int main() {
    const uint8_t key[32] = {0};      // 256-bit key (all zero for demo)
    const uint8_t nonce[8] = {0};     // 64-bit nonce (all zero for demo)
    char plaintext[64];

    printf("Enter plaintext: ");
    fgets(plaintext, sizeof(plaintext), stdin);

    size_t len = strlen(plaintext);
    if (plaintext[len - 1] == '\n') plaintext[--len] = '\0';

    uint8_t ciphertext[64], decrypted[64];

    // Encrypt
    salsa20_encrypt(key, nonce, (uint8_t *)plaintext, ciphertext, len);

    printf("Ciphertext (hex): ");
    for (size_t i = 0; i < len; ++i)
        printf("%02x", ciphertext[i]);
    printf("\n");

    // Decrypt
    salsa20_encrypt(key, nonce, ciphertext, decrypted, len);
    decrypted[len] = '\0';

    printf("Decrypted text: %s\n", decrypted);

    return 0;
}
