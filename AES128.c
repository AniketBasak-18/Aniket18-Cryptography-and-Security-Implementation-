#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>  // For __rdtsc()

#define Nb 4
#define Nk 4
#define Nr 10

typedef struct {
    uint32_t round_keys[4*(Nr+1)];
} aes128_state_t;

// AES S-box
static const uint8_t sbox[256] = {
    /* 0x00 - 0x0f */ 0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    /* 0x10 - 0x1f */ 0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    /* ... fill remaining 224 entries ... */
    // To save space in this example, you would fill the rest of sbox from standard AES S-box table.
};

// AES key expansion (simplified)
void aes128_key_expansion(const uint8_t *key, aes128_state_t *state) {
    uint32_t *w = state->round_keys;
    for (int i = 0; i < Nk; ++i) {
        w[i] = (key[4*i]<<24) | (key[4*i+1]<<16) | (key[4*i+2]<<8) | key[4*i+3];
    }
    uint32_t temp;
    for (int i = Nk; i < Nb*(Nr+1); ++i) {
        temp = w[i-1];
        if (i % Nk == 0) {
            // RotWord + SubWord + Rcon
            temp = (sbox[(temp>>16)&0xff]<<24) | (sbox[(temp>>8)&0xff]<<16) |
                   (sbox[temp&0xff]<<8) | sbox[(temp>>24)&0xff];
            // Rcon simplified
            temp ^= (0x01 << 24);
        }
        w[i] = w[i-Nk] ^ temp;
    }
}

// AES-128 block encryption (no fancy optimizations for readability)
void aes128_encrypt_block(aes128_state_t *state, uint8_t *block) {
    uint32_t *rk = state->round_keys;
    uint8_t s[16];
    memcpy(s, block, 16);

    // Initial AddRoundKey
    for (int i = 0; i < 16; ++i) s[i] ^= ((uint8_t*)rk)[i];

    // Simplified: full rounds would go here (SubBytes, ShiftRows, MixColumns, AddRoundKey)
    // For demo, just one more AddRoundKey to simulate work:
    for (int i = 0; i < 16; ++i) s[i] ^= ((uint8_t*)rk)[16];

    memcpy(block, s, 16);
}

// Encrypt full buffer (1 MB)
void aes128_encrypt_buffer(aes128_state_t *state, uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i += 16) {
        aes128_encrypt_block(state, data + i);
    }
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
    aes128_state_t state;
    size_t data_len = 1024 * 1024;  // 1 MB
    uint8_t *data = malloc(data_len);
    uint8_t key[16];  // 16-byte key

    if (!data) {
        perror("Failed to allocate memory");
        return 1;
    }

    const int runs = 10000;  // Fewer runs on Windows
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
    for (int i = 0; i < 16; ++i) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    printf("Data size: %zu bytes\n", data_len);
    printf("Total runs: %d\n", runs);
    printf("Average cycles (AES only): %.2f\n", avg_cycles);
    printf("Average cycles per byte: %.2f\n", avg_cycles / data_len);

    free(data);
    return 0;
}
