#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>  // For __rdtsc()

// Salsa20 parameters
#define SALSA_ROUNDS 20  // Standard = 20 rounds

typedef struct {
    uint32_t input[16]; // state: constants, key, counter, nonce
} salsa20_state_t;

// Salsa20 constants: "expand 32-byte k"
static const uint32_t salsa_constants[4] = {
    0x61707865u, 0x3320646eu, 0x79622d32u, 0x6b206574u
};

static uint32_t u8to32(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void u32to8(uint32_t v, uint8_t *p) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

#define ROTL32(v, c) (((v) << (c)) | ((v) >> (32 - (c))))

// Quarterround for Salsa20
static void salsa_quarterround(uint32_t *y0, uint32_t *y1, uint32_t *y2, uint32_t *y3) {
    *y1 ^= ROTL32(*y0 + *y3, 7);
    *y2 ^= ROTL32(*y1 + *y0, 9);
    *y3 ^= ROTL32(*y2 + *y1, 13);
    *y0 ^= ROTL32(*y3 + *y2, 18);
}

// Salsa20 double round (column round + row round)
static void salsa_doubleround(uint32_t x[16]) {
    // Column round
    salsa_quarterround(&x[0], &x[4], &x[8], &x[12]);
    salsa_quarterround(&x[5], &x[9], &x[13], &x[1]);
    salsa_quarterround(&x[10], &x[14], &x[2], &x[6]);
    salsa_quarterround(&x[15], &x[3], &x[7], &x[11]);

    // Row round
    salsa_quarterround(&x[0], &x[1], &x[2], &x[3]);
    salsa_quarterround(&x[5], &x[6], &x[7], &x[4]);
    salsa_quarterround(&x[10], &x[11], &x[8], &x[9]);
    salsa_quarterround(&x[15], &x[12], &x[13], &x[14]);
}

static void salsa20_block(uint8_t out[64], const uint32_t in[16]) {
    uint32_t x[16];
    for (int i = 0; i < 16; ++i) x[i] = in[i];

    for (int i = 0; i < SALSA_ROUNDS/2; ++i) {
        salsa_doubleround(x);
    }

    for (int i = 0; i < 16; ++i) x[i] += in[i];
    for (int i = 0; i < 16; ++i) u32to8(x[i], out + 4*i);
}

void salsa20_init(salsa20_state_t *st, const uint8_t key[32], const uint8_t nonce[8], uint64_t counter) {
    // Constants
    st->input[0]  = salsa_constants[0];
    st->input[5]  = salsa_constants[1];
    st->input[10] = salsa_constants[2];
    st->input[15] = salsa_constants[3];

    // Key (32 bytes = 8 words)
    for (int i = 0; i < 4; ++i) {
        st->input[1 + i]  = u8to32(key + 4*i);
        st->input[11 + i] = u8to32(key + 16 + 4*i);
    }

    // Counter (64-bit) + Nonce (64-bit)
    st->input[6] = (uint32_t)(counter & 0xffffffffu);
    st->input[7] = (uint32_t)(counter >> 32);
    st->input[8] = u8to32(nonce + 0);
    st->input[9] = u8to32(nonce + 4);
}

void salsa20_encrypt_buffer(salsa20_state_t *st, uint8_t *data, size_t len) {
    uint8_t keystream[64];
    size_t pos = 0;
    while (pos < len) {
        salsa20_block(keystream, st->input);
        size_t take = (len - pos > 64) ? 64 : (len - pos);
        for (size_t j = 0; j < take; ++j) {
            data[pos + j] ^= keystream[j];
        }
        pos += take;

        // increment 64-bit block counter
        if (++st->input[6] == 0) {
            st->input[7]++;
        }
    }
}

// Simple LCG PRNG for benchmarking
static uint32_t lcg_seed = 987654321;
uint32_t lcg_rand() {
    lcg_seed = (1103515245u * lcg_seed + 12345u) & 0x7fffffffu;
    return lcg_seed;
}
void generate_random(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) buf[i] = (uint8_t)(lcg_rand() & 0xffu);
}

int main() {
    salsa20_state_t state;
    size_t data_len = 1024 * 1024; // 1 MB
    uint8_t *data = malloc(data_len);
    uint8_t key[32];   // 32-byte key
    uint8_t nonce[8];  // 64-bit nonce

    if (!data) {
        perror("Failed to allocate memory");
        return 1;
    }

    const int runs = 10000;
    uint64_t total_cycles = 0;

    for (int i = 0; i < runs; ++i) {
        generate_random(data, data_len);       // plaintext
        generate_random(key, sizeof(key));    // key
        generate_random(nonce, sizeof(nonce));// nonce

        salsa20_init(&state, key, nonce, 0ull);

        uint64_t start = __rdtsc();
        salsa20_encrypt_buffer(&state, data, data_len);
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
    printf("Average cycles (Salsa20 only): %.2f\n", avg_cycles);
    printf("Average cycles per byte: %.2f\n", avg_cycles / data_len);

    free(data);
    return 0;
}
