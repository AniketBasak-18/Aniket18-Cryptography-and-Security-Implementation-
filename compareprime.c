#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <x86intrin.h>  // for __rdtsc()

#define RUNS 10000
#define PRIME_BITS 512
#define ITERATIONS 25   // number of iterations for MR and SS

//------------------------------------------------------------
// Modular exponentiation (for Miller-Rabin & Solovay-Strassen)
//------------------------------------------------------------
void modexp(mpz_t result, const mpz_t base, const mpz_t exp, const mpz_t mod) {
    mpz_t r, b, e;
    mpz_inits(r, b, e, NULL);
    mpz_set_ui(r, 1);
    mpz_set(b, base);
    mpz_set(e, exp);

    while (mpz_cmp_ui(e, 0) > 0) {
        if (mpz_odd_p(e)) {
            mpz_mul(r, r, b);
            mpz_mod(r, r, mod);
        }
        mpz_mul(b, b, b);
        mpz_mod(b, b, mod);
        mpz_fdiv_q_2exp(e, e, 1); // e >>= 1
    }
    mpz_set(result, r);
    mpz_clears(r, b, e, NULL);
}

//------------------------------------------------------------
// One iteration of Miller-Rabin
//------------------------------------------------------------
int miller_rabin_once(const mpz_t n, gmp_randstate_t state) {
    if (mpz_cmp_ui(n, 2) < 0) return 0;
    if (mpz_even_p(n)) return 0;

    mpz_t n_minus1, d, a, x;
    mpz_inits(n_minus1, d, a, x, NULL);
    mpz_sub_ui(n_minus1, n, 1);
    mpz_set(d, n_minus1);

    unsigned long s = 0;
    while (mpz_even_p(d)) {
        mpz_fdiv_q_2exp(d, d, 1);
        s++;
    }

    mpz_urandomm(a, state, n_minus1);
    mpz_add_ui(a, a, 2); // random in [2, n-2]

    modexp(x, a, d, n);
    if (mpz_cmp_ui(x, 1) == 0 || mpz_cmp(x, n_minus1) == 0) {
        mpz_clears(n_minus1, d, a, x, NULL);
        return 1;
    }

    for (unsigned long r = 1; r < s; r++) {
        mpz_mul(x, x, x);
        mpz_mod(x, x, n);
        if (mpz_cmp(x, n_minus1) == 0) {
            mpz_clears(n_minus1, d, a, x, NULL);
            return 1;
        }
    }

    mpz_clears(n_minus1, d, a, x, NULL);
    return 0;
}

//------------------------------------------------------------
// One iteration of Solovay–Strassen
//------------------------------------------------------------
int solovay_strassen_once(const mpz_t n, gmp_randstate_t state) {
    if (mpz_cmp_ui(n, 2) < 0) return 0;
    if (mpz_even_p(n)) return 0;

    mpz_t a, exp, res, g;
    mpz_inits(a, exp, res, g, NULL);

    mpz_urandomm(a, state, n);
    mpz_add_ui(a, a, 1); // random in [1, n-1]

    mpz_gcd(g, a, n);
    if (mpz_cmp_ui(g, 1) != 0) {
        mpz_clears(a, exp, res, g, NULL);
        return 0;
    }

    mpz_sub_ui(exp, n, 1);
    mpz_fdiv_q_2exp(exp, exp, 1);
    modexp(res, a, exp, n);

    int jac = mpz_jacobi(a, n);
    if (jac == -1) {
        mpz_add(res, res, n);
    }

    int result = (mpz_cmp_si(res, jac) == 0);
    mpz_clears(a, exp, res, g, NULL);
    return result;
}

//------------------------------------------------------------
// Run MR with ITERATIONS rounds
//------------------------------------------------------------
int miller_rabin_test(const mpz_t n, gmp_randstate_t state) {
    for (int i = 0; i < ITERATIONS; i++) {
        if (!miller_rabin_once(n, state)) return 0;
    }
    return 1;
}

//------------------------------------------------------------
// Run SS with ITERATIONS rounds
//------------------------------------------------------------
int solovay_strassen_test(const mpz_t n, gmp_randstate_t state) {
    for (int i = 0; i < ITERATIONS; i++) {
        if (!solovay_strassen_once(n, state)) return 0;
    }
    return 1;
}

//------------------------------------------------------------
// Benchmark
//------------------------------------------------------------
int main() {
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    mpz_t n;
    mpz_init(n);

    unsigned long long total_mr = 0, total_ss = 0, total_gmp = 0;

    for (int i = 0; i < RUNS; i++) {
        mpz_urandomb(n, state, PRIME_BITS);
        mpz_setbit(n, 0);              // make it odd
        mpz_setbit(n, PRIME_BITS - 1); // ensure bit length

        unsigned long long start, end;

        // Miller-Rabin
        start = __rdtsc();
        miller_rabin_test(n, state);
        end = __rdtsc();
        total_mr += (end - start);

        // Solovay–Strassen
        start = __rdtsc();
        solovay_strassen_test(n, state);
        end = __rdtsc();
        total_ss += (end - start);

        // GMP built-in
        start = __rdtsc();
        mpz_probab_prime_p(n, ITERATIONS);
        end = __rdtsc();
        total_gmp += (end - start);
    }

    printf("Average cycles over %d runs (random %d-bit numbers, %d iterations):\n", 
            RUNS, PRIME_BITS, ITERATIONS);
    printf(" Miller-Rabin     : %llu cycles\n", total_mr / RUNS);
    printf(" Solovay-Strassen : %llu cycles\n", total_ss / RUNS);
    printf(" GMP library      : %llu cycles\n", total_gmp / RUNS);

    mpz_clear(n);
    gmp_randclear(state);
    return 0;
}
