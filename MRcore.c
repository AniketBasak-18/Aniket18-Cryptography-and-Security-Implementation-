#include <gmp.h>
#include <stdlib.h>
#include <time.h>

// Miller-Rabin core function: returns 1 if n is probably prime, 0 if composite
int miller_rabin_core(const mpz_t n, gmp_randstate_t state) {
    if (mpz_cmp_ui(n, 2) < 0) return 0; // n < 2 is not prime
    if (mpz_even_p(n)) return (mpz_cmp_ui(n, 2) == 0); // 2 is prime

    mpz_t n_minus1, d, a, x, tmp;
    unsigned long s = 0;

    mpz_inits(n_minus1, d, a, x, tmp, NULL);
    mpz_sub_ui(n_minus1, n, 1); // n-1
    mpz_set(d, n_minus1);

    // Factor out powers of 2 from n-1: n-1 = d * 2^s
    while (mpz_even_p(d)) {
        mpz_fdiv_q_2exp(d, d, 1); // d /= 2
        s++;
    }

    // Choose random base a in [2, n-2]
    mpz_urandomm(a, state, n_minus1); // a in [0, n-2]
    mpz_add_ui(a, a, 2);              // shift to [2, n-2]

    // x = a^d mod n
    mpz_powm(x, a, d, n);

    if (mpz_cmp_ui(x, 1) == 0 || mpz_cmp(x, n_minus1) == 0) {
        mpz_clears(n_minus1, d, a, x, tmp, NULL);
        return 1; // probably prime
    }

    for (unsigned long r = 1; r < s; r++) {
        mpz_powm_ui(x, x, 2, n); // x = x^2 mod n
        if (mpz_cmp(x, n_minus1) == 0) {
            mpz_clears(n_minus1, d, a, x, tmp, NULL);
            return 1; // probably prime
        }
    }

    // Composite
    mpz_clears(n_minus1, d, a, x, tmp, NULL);
    return 0;
}
