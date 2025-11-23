#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>  // For __rdtsc()
#include <wmmintrin.h>  // For AES-NI intrinsics

#define AES_BLOCK_SIZE 16
#define AES_ROUNDS 10

typedef struct {
    __m128i round_keys[AES_ROUNDS + 1];
} aes128_state_t;

// AES-128 key expansion using AES-NI
void aes128_key_expansion(const uint8_t *key, aes128_state_t *state) {
    __m128i temp1, temp2;
    temp1 = _mm_loadu_si128((const __m128i*)key);
    state->round_keys[0] = temp1;

    #define AES_128_ASSIST(t1, t2, i) \
        t2 = _mm_aeskeygenassist_si128(t1, i); \
        t2 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(3,3,3,3)); \
        t1 = _mm_xor_si128(t1, _mm_slli_si128(t1,4)); \
        t1 = _mm_xor_si128(t1, _mm_slli_si128(t1,4)); \
        t1 = _mm_xor_si128(t1, _mm_slli_si128(t1,4)); \
        t1 = _mm_xor_si128(t1, t2);

    AES_128_ASSIST(temp1, temp2, 0x01); state->round_keys[1] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x02); state->round_keys[2] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x04); state->round_keys[3] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x08); state->round_keys[4] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x10); state->round_keys[5] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x20); state->round_keys[6] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x40); state->round_keys[7] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x80); state->round_keys[8] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x1B); state->round_keys[9] = temp1;
    AES_128_ASSIST(temp1, temp2, 0x36); state->round_keys[10] = temp1;

    #undef AES_128_ASSIST
}

// AES-128 block encryption using AES-NI
static inline void aes128_encrypt_block(aes128_state_t *state, uint8_t *block) {
    __m128i m = _mm_loadu_si128((__m128i*)block);
    m = _mm_xor_si128(m, state->round_keys[0]);
    for (int i = 1; i < AES_ROUNDS; ++i) {
        m = _mm_aesenc_si128(m, state->round_keys[i]);
    }
    m = _mm_aesenclast_si128(m, state->round_keys[AES_ROUNDS]);
    _mm_storeu_si128((__m128i*)block, m);
}

// Encrypt full buffer (1 MB)
void aes128_encrypt_buffer(aes128_state_t *state, uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i += AES_BLOCK_SIZE) {
        aes128_encrypt_block(state, data + i);
    }
}

// Simple LCG for pseudo-random values
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
    aes128_state_t state;
    size_t data_len = 1024 * 1024;  // 1 MB
    uint8_t *data = malloc(data_len);
    uint8_t key[16];  // 16-byte key

    if (!data) {
        perror("Failed to allocate memory");
        return 1;
    }

    const int runs = 10000;  // fewer runs for AES-NI (faster)
    uint64_t total_cycles = 0;

    for (int i = 0; i < runs; ++i) {
        generate_random(data, data_len);     // Random plaintext
        generate_random(key, sizeof(key));   // Random key
        aes128_key_expansion(key, &state);   // Key schedule

        uint64_t start = __rdtsc();
        aes128_encrypt_buffer(&state, data, data_len);
        uint64_t end = __rdtsc();

        total_cycles += (end - start);
    }

    double avg_cycles = (double)total_cycles / runs;

    printf("Sample encrypted output (first 16 bytes): ");
    for (int i = 0; i < 16; ++i) printf("%02x ", data[i]);
    printf("\n");

    printf("Data size: %zu bytes\n", data_len);
    printf("Total runs: %d\n", runs);
    printf("Average cycles (AES only): %.2f\n", avg_cycles);
    printf("Average cycles per byte: %.2f\n", avg_cycles / data_len);

    free(data);
    return 0;
}
