#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Miller-Rabin test for a single base
int miller_rabin_single(mpz_t n, gmp_randstate_t state) {
    mpz_t a, n_minus1, d, x;
    mpz_inits(a, n_minus1, d, x, NULL);
    
    mpz_sub_ui(n_minus1, n, 1);
    mpz_set(d, n_minus1);

    unsigned long r = 0;
    while (mpz_even_p(d)) {
        mpz_fdiv_q_2exp(d, d, 1);
        r++;
    }

    // choose random base a in [2, n-2]
    mpz_urandomm(a, state, n_minus1);
    mpz_add_ui(a, a, 2);

    // x = a^d mod n
    mpz_powm(x, a, d, n);

    if (mpz_cmp_ui(x, 1) == 0 || mpz_cmp(x, n_minus1) == 0) {
        mpz_clears(a, n_minus1, d, x, NULL);
        return 1; // probably prime
    }

    for (unsigned long i = 1; i < r; i++) {
        mpz_powm_ui(x, x, 2, n);
        if (mpz_cmp(x, n_minus1) == 0) {
            mpz_clears(a, n_minus1, d, x, NULL);
            return 1; // probably prime
        }
    }

    mpz_clears(a, n_minus1, d, x, NULL);
    return 0; // composite
}

int main() {
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_randseed_ui(state, time(NULL));

    mpz_t p, q, n;
    mpz_inits(p, q, n, NULL);

    // Generate 256-bit primes p and q
    mpz_urandomb(p, state, 256);
    mpz_nextprime(p, p);
    mpz_urandomb(q, state, 256);
    mpz_nextprime(q, q);

    // Compute n = p * q
    mpz_mul(n, p, q);

    printf("Generated n (composite, 512-bit):\n");
    mpz_out_str(stdout, 10, n);
    printf("\n\n");

    long rounds = 100000, false_prime = 0;
    for (long i = 0; i < rounds; i++) {
        if (miller_rabin_single(n, state))
            false_prime++;
    }

    printf("Out of %ld rounds, falsely declared prime: %ld times\n", rounds, false_prime);
    printf("Experimental probability ≈ %.10f\n", (double)false_prime / rounds);

    mpz_clears(p, q, n, NULL);
    gmp_randclear(state);
    return 0;
}
