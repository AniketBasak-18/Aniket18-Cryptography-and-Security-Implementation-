// mr_512_cycles_nogmp.c
// Miller-Rabin test for 512-bit numbers without GMP
// Works in Windows VS Code (MSVC or MinGW)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <intrin.h>

#define RUNS   10000
#define LIMBS  8         // 512 bits / 64 bits
#define ROUNDS 10        // Miller-Rabin rounds

typedef struct {
    uint64_t v[LIMBS]; // little-endian (v[0] = least significant limb)
} Big512;

// --- Utility functions ---

// Set x = 0
void big_zero(Big512 *x) {
    memset(x->v, 0, sizeof(x->v));
}

// Compare (return -1,0,1)
int big_cmp(const Big512 *a, const Big512 *b) {
    for (int i = LIMBS-1; i >= 0; i--) {
        if (a->v[i] < b->v[i]) return -1;
        if (a->v[i] > b->v[i]) return 1;
    }
    return 0;
}

// Copy
void big_copy(Big512 *dst, const Big512 *src) {
    memcpy(dst->v, src->v, sizeof(dst->v));
}

// Check even
int big_even(const Big512 *a) {
    return !(a->v[0] & 1ULL);
}

// Subtract b from a (assume a>=b)
void big_sub(Big512 *res, const Big512 *a, const Big512 *b) {
    unsigned __int128 carry = 0;
    for (int i = 0; i < LIMBS; i++) {
        unsigned __int128 tmp = (unsigned __int128)a->v[i] - b->v[i] - carry;
        res->v[i] = (uint64_t)tmp;
        carry = (tmp >> 64) & 1ULL;
    }
}

// Right shift by 1
void big_rshift1(Big512 *x) {
    uint64_t carry = 0;
    for (int i = LIMBS-1; i >= 0; i--) {
        uint64_t newcarry = x->v[i] & 1ULL;
        x->v[i] = (x->v[i] >> 1) | (carry << 63);
        carry = newcarry;
    }
}

// Generate random 512-bit odd with MSB set
void big_rand(Big512 *x) {
    for (int i = 0; i < LIMBS; i++) {
        uint64_t r = (((uint64_t)rand() << 32) ^ rand());
        r = (r << 32) ^ rand();
        x->v[i] = r;
    }
    x->v[LIMBS-1] |= (1ULL << 63); // force MSB
    x->v[0] |= 1ULL;               // make odd
}

// Print hex
void big_print(const Big512 *x) {
    for (int i = LIMBS-1; i >= 0; i--)
        printf("%016llx", (unsigned long long)x->v[i]);
    printf("\n");
}

// Modular multiplication (naive 512-bit * 512-bit % mod)
void big_mulmod(Big512 *res, const Big512 *a, const Big512 *b, const Big512 *mod) {
    __uint128_t tmp[2*LIMBS];
    memset(tmp, 0, sizeof(tmp));

    // multiply
    for (int i = 0; i < LIMBS; i++) {
        __uint128_t carry = 0;
        for (int j = 0; j < LIMBS; j++) {
            __uint128_t t = tmp[i+j] + (__uint128_t)a->v[i] * b->v[j] + carry;
            tmp[i+j] = t & 0xFFFFFFFFFFFFFFFFULL;
            carry = t >> 64;
        }
        tmp[i+LIMBS] += carry;
    }

    // Reduce modulo mod (simple subtraction method, not optimized)
    Big512 r;
    for (int i = 0; i < LIMBS; i++) r.v[i] = (uint64_t)tmp[i];
    while (big_cmp(&r, mod) >= 0) {
        big_sub(&r, &r, mod);
    }
    big_copy(res, &r);
}

// Modular exponentiation: res = base^exp mod mod
void big_powmod(Big512 *res, const Big512 *base, const Big512 *exp, const Big512 *mod) {
    Big512 result, b, e;
    big_zero(&result); result.v[0] = 1;
    big_copy(&b, base);
    big_copy(&e, exp);

    for (int i = 0; i < LIMBS*64; i++) {
        if (e.v[i/64] & (1ULL << (i%64))) {
            big_mulmod(&result, &result, &b, mod);
        }
        big_mulmod(&b, &b, &b, mod);
    }
    big_copy(res, &result);
}

// Miller-Rabin primality test
int miller_rabin(const Big512 *n, int rounds) {
    if (n->v[0] % 2 == 0) return 0; // even

    // n-1 = d * 2^s
    Big512 n_minus1, d;
    big_copy(&n_minus1, n);
    Big512 one; big_zero(&one); one.v[0] = 1;
    big_sub(&n_minus1, n, &one);
    big_copy(&d, &n_minus1);

    int s = 0;
    while (big_even(&d)) {
        big_rshift1(&d);
        s++;
    }

    for (int i = 0; i < rounds; i++) {
        Big512 a;
        big_rand(&a);
        // a = a % (n-2) + 2
        if (big_cmp(&a, &n_minus1) >= 0) big_sub(&a, &a, &n_minus1);

        Big512 x;
        big_powmod(&x, &a, &d, n);
        if (big_cmp(&x, &one) == 0 || big_cmp(&x, &n_minus1) == 0) continue;

        int cont = 0;
        for (int r = 1; r < s; r++) {
            big_mulmod(&x, &x, &x, n);
            if (big_cmp(&x, &n_minus1) == 0) { cont = 1; break; }
        }
        if (!cont) return 0;
    }
    return 1;
}

int main() {
    srand((unsigned)time(NULL));

    uint64_t min_cycles = UINT64_MAX, max_cycles = 0;
    long double sum_cycles = 0.0;
    int probable_primes = 0;

    for (int i = 0; i < RUNS; i++) {
        Big512 n;
        big_rand(&n);

        uint64_t t0 = __rdtsc();
        int is_prime = miller_rabin(&n, ROUNDS);
        uint64_t t1 = __rdtsc();

        uint64_t cycles = t1 - t0;
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles) max_cycles = cycles;
        sum_cycles += cycles;
        probable_primes += is_prime;
    }

    printf("Miller–Rabin on %d random 512-bit numbers:\n", RUNS);
    printf("  Min cycles: %llu\n", (unsigned long long)min_cycles);
    printf("  Max cycles: %llu\n", (unsigned long long)max_cycles);
    printf("  Avg cycles: %.0Lf\n", sum_cycles / RUNS);
    printf("  Probable primes: %d\n", probable_primes);
    return 0;
}
