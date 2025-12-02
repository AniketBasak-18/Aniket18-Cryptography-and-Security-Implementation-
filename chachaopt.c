#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>  // For __rdtsc()

// ChaCha20 parameters
#define CHACHA_ROUNDS 20  // 20 = 10 double-rounds

typedef struct {
    uint32_t input[16]; // state: constants, key, counter, nonce
} chacha20_state_t;

// ChaCha constant words: "expand 32-byte k"
static const uint32_t chacha_constants[4] = {
    0x61707865u, 0x3320646eu, 0x79622d32u, 0x6b206574u
};

static uint32_t u8to32(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void u32to8(uint32_t v, uint8_t *p) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

#define ROTL32(v, c) (((v) << (c)) | ((v) >> (32 - (c))))

static void chacha_quarterround(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *a += *b; *d ^= *a; *d = ROTL32(*d, 16);
    *c += *d; *b ^= *c; *b = ROTL32(*b, 12);
    *a += *b; *d ^= *a; *d = ROTL32(*d, 8);
    *c += *d; *b ^= *c; *b = ROTL32(*b, 7);
}

static void chacha_doubleround(uint32_t x[16]) {
    // Column round
    chacha_quarterround(&x[0], &x[4], &x[8], &x[12]);
    chacha_quarterround(&x[1], &x[5], &x[9], &x[13]);
    chacha_quarterround(&x[2], &x[6], &x[10], &x[14]);
    chacha_quarterround(&x[3], &x[7], &x[11], &x[15]);
    // Diagonal round
    chacha_quarterround(&x[0], &x[5], &x[10], &x[15]);
    chacha_quarterround(&x[1], &x[6], &x[11], &x[12]);
    chacha_quarterround(&x[2], &x[7], &x[8], &x[13]);
    chacha_quarterround(&x[3], &x[4], &x[9], &x[14]);
}

static void chacha20_block(uint8_t out[64], const uint32_t in[16]) {
    uint32_t x[16];
    for (int i = 0; i < 16; ++i) x[i] = in[i];
    for (int i = 0; i < CHACHA_ROUNDS/2; ++i) {
        chacha_doubleround(x);
    }
    for (int i = 0; i < 16; ++i) x[i] += in[i];
    for (int i = 0; i < 16; ++i) u32to8(x[i], out + 4*i);
}

void chacha20_init(chacha20_state_t *st, const uint8_t key[32], const uint8_t nonce[12], uint32_t counter) {
    // constants
    st->input[0] = chacha_constants[0];
    st->input[1] = chacha_constants[1];
    st->input[2] = chacha_constants[2];
    st->input[3] = chacha_constants[3];
    // key (32 bytes -> 8 words)
    for (int i = 0; i < 8; ++i) {
        st->input[4 + i] = u8to32(key + 4*i);
    }
    // counter and nonce
    st->input[12] = counter;             // 32-bit block counter
    st->input[13] = u8to32(nonce + 0);
    st->input[14] = u8to32(nonce + 4);
    st->input[15] = u8to32(nonce + 8);
}

void chacha20_encrypt_buffer(chacha20_state_t *st, uint8_t *data, size_t len) {
    uint8_t keystream[64];
    size_t pos = 0;
    while (pos < len) {
        chacha20_block(keystream, st->input);
        size_t take = (len - pos > 64) ? 64 : (len - pos);
        for (size_t j = 0; j < take; ++j) {
            data[pos + j] ^= keystream[j];
        }
        pos += take;
        // increment 32-bit block counter (wrap behavior is natural)
        st->input[12]++;
    }
}

// Simple LCG for pseudo-random data (same as your AES example)
static uint32_t lcg_seed = 123456789;
uint32_t lcg_rand() {
    lcg_seed = (1103515245u * lcg_seed + 12345u) & 0x7fffffffu;
    return lcg_seed;
}
void generate_random(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) buf[i] = (uint8_t)(lcg_rand() & 0xffu);
}

int main() {
    chacha20_state_t state;
    size_t data_len = 1024 * 1024; // 1 MB
    uint8_t *data = malloc(data_len);
    uint8_t key[32];   // 32-byte key for ChaCha20
    uint8_t nonce[12]; // 96-bit nonce

    if (!data) {
        perror("Failed to allocate memory");
        return 1;
    }

    const int runs = 10000;  // match AES example's run count
    uint64_t total_cycles = 0;

    for (int i = 0; i < runs; ++i) {
        generate_random(data, data_len);       // random plaintext
        generate_random(key, sizeof(key));    // random key
        generate_random(nonce, sizeof(nonce));// random nonce

        // Initialize state with counter = 0
        chacha20_init(&state, key, nonce, 0u);

        uint64_t start = __rdtsc();
        chacha20_encrypt_buffer(&state, data, data_len);
        uint64_t end = __rdtsc();

        total_cycles += (end - start);
    }

    double avg_cycles = (double)total_cycles / runs;

    printf("Sample encrypted output (first 16 bytes): ");
    for (int i = 0; i < 16; ++i) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    printf("Data size: %zu bytes\n", data_len);
    printf("Total runs: %d\n", runs);
    printf("Average cycles (ChaCha20 only): %.2f\n", avg_cycles);
    printf("Average cycles per byte: %.2f\n", avg_cycles / data_len);

    free(data);
    return 0;
}
