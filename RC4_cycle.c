#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <x86intrin.h>  // For __rdtsc()

// Macro for one PRGA step
#define RC4_STEP(i, j, S, data, idx) do { \
    i += 1; \
    j += S[i]; \
    tmp = S[i]; S[i] = S[j]; S[j] = tmp; \
    data[idx++] ^= S[(uint8_t)(S[i] + S[j])]; \
} while (0)

typedef struct {
    uint8_t S[256];
    uint8_t i;
    uint8_t j;
} rc4_state_t;

void rc4_init(rc4_state_t *state, const uint8_t *key, size_t keylen) {
    uint8_t j = 0, tmp;
    for (uint16_t i = 0; i < 256; ++i) {
        state->S[i] = (uint8_t)i;
    }
    for (uint16_t i = 0; i < 256; ++i) {
        j += state->S[i] + key[i % keylen];
        tmp = state->S[i];
        state->S[i] = state->S[j];
        state->S[j] = tmp;
    }
    state->i = 0;
    state->j = 0;
}

void rc4_crypt(rc4_state_t *state, uint8_t *data, size_t len) {
    uint8_t i = state->i;
    uint8_t j = state->j;
    uint8_t tmp;
    size_t idx = 0;

    // Unrolled loop (16x) for speed
    size_t blocks = len / 16;
    for (size_t b = 0; b < blocks; ++b) {
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
        RC4_STEP(i, j, state->S, data, idx); RC4_STEP(i, j, state->S, data, idx);
    }

    // Remaining bytes
    size_t remain = len % 16;
    for (size_t r = 0; r < remain; ++r) {
        RC4_STEP(i, j, state->S, data, idx);
    }

    state->i = i;
    state->j = j;
}

// Simple LCG for generating pseudo-random values
static uint32_t lcg_seed = 123456789;
uint32_t lcg_rand() {
    lcg_seed = (1103515245 * lcg_seed + 12345) & 0x7fffffff;
    return lcg_seed;
}

void generate_random(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (uint8_t)(lcg_rand() & 0xff);
    }
}

int main() {
    rc4_state_t state;
    size_t data_len = 1024 * 1024;  // 1 MB
    uint8_t *data = malloc(data_len);
    uint8_t key[16];  // 16-byte key

    if (!data) {
        perror("Failed to allocate memory");
        return 1;
    }

    const int runs = 10000;  // Fewer runs for quicker test on Windows
    uint64_t total_cycles = 0;

    for (int i = 0; i < runs; ++i) {
        generate_random(data, data_len);     // Random plaintext
        generate_random(key, sizeof(key));   // Random key
        rc4_init(&state, key, sizeof(key));  // KSA

        uint64_t start = __rdtsc();
        rc4_crypt(&state, data, data_len);   // PRGA
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
    printf("Average cycles (PRGA only): %.2f\n", avg_cycles);
    printf("Average cycles per byte: %.2f\n", avg_cycles / data_len);

    free(data);
    return 0;
}
