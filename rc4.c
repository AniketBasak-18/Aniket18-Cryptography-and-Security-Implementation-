#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>   // Windows API for affinity & priorities
#include <x86intrin.h> // __rdtsc

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

    // Unrolled loop (16x) for maximum performance
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

    // Handle remaining bytes (0-15)
    size_t remain = len % 16;
    for (size_t r = 0; r < remain; ++r) {
        RC4_STEP(i, j, state->S, data, idx);
    }

    state->i = i;
    state->j = j;
}

// Windows version of setup_no_interruptions
void setup_no_interruptions() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numCores = sysInfo.dwNumberOfProcessors;

    if (numCores < 2) {
        fprintf(stderr, "System has only 1 core; cannot assign a separate core.\n");
        exit(1);
    }

    // Pin thread to last CPU core
    HANDLE hThread = GetCurrentThread();
    DWORD_PTR mask = (DWORD_PTR)1 << (numCores - 1);
    if (SetThreadAffinityMask(hThread, mask) == 0) {
        perror("Failed to set CPU affinity");
        exit(1);
    }

    // Set highest priority possible
    if (!SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL)) {
        perror("Failed to set thread priority");
        exit(1);
    }

    // No direct mlockall equivalent in Windows; VirtualLock can lock memory region if needed
}

// Simple LCG for generating unique keys and plaintexts
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
    setup_no_interruptions();

    rc4_state_t state;
    size_t data_len = 1024 * 1024;  // 1 MB
    uint8_t *data = (uint8_t*)malloc(data_len);
    uint8_t key[16];  // 16-byte key
    if (!data) {
        perror("Failed to allocate memory");
        exit(1);
    }

    const int runs = 10000;
    uint64_t total_cycles = 0;

    // Warm-up run to cache data/instructions
    generate_random(data, data_len);
    generate_random(key, sizeof(key));
    rc4_init(&state, key, sizeof(key));
    rc4_crypt(&state, data, data_len);

    for (int i = 0; i < runs; ++i) {
        generate_random(data, data_len);
        generate_random(key, sizeof(key));
        rc4_init(&state, key, sizeof(key));

        uint64_t start = __rdtsc();
        rc4_crypt(&state, data, data_len);
        uint64_t end = __rdtsc();
        total_cycles += (end - start);

        if ((i + 1) % 100000 == 0) {
            printf("Completed %d runs\n", i + 1);
            fflush(stdout);
        }
    }

    double avg_cycles = (double)total_cycles / runs;

    printf("Last encrypted data sample (first 16 bytes, hex): ");
    for (size_t i = 0; i < 16; ++i) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    printf("Data size: %zu bytes\n", data_len);
    printf("Total runs: %d\n", runs);
    printf("Average cycles (rdtsc, PRGA burst only): %.2f\n", avg_cycles);
    printf("Average cycles per byte: %.2f\n", avg_cycles / data_len);

    free(data);
    return 0;
}
