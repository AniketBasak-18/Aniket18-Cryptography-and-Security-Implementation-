#include <stdio.h>
#include <gmp.h>
#include <stdlib.h>

// Fraction struct: k/d
typedef struct {
    mpz_t k;
    mpz_t d;
} Fraction;

// Continued fraction of e/N
int continued_fraction(mpz_t e, mpz_t N, mpz_t *cf, int max_terms) {
    mpz_t q, r, a, temp_e, temp_N;
    mpz_inits(q, r, a, temp_e, temp_N, NULL);
    mpz_set(temp_e, e);
    mpz_set(temp_N, N);

    int i = 0;
    while (mpz_cmp_ui(temp_N, 0) != 0 && i < max_terms) {
        mpz_fdiv_qr(q, r, temp_e, temp_N);
        mpz_init_set(cf[i], q);
        i++;
        mpz_set(temp_e, temp_N);
        mpz_set(temp_N, r);
    }

    mpz_clears(q, r, a, temp_e, temp_N, NULL);
    return i;
}

// Generate convergents from continued fraction
void generate_convergents(mpz_t *cf, int len, Fraction *convs) {
    mpz_t num1, num2, den1, den2, num, den, tmp;
    mpz_inits(num1, num2, den1, den2, num, den, tmp, NULL);
    mpz_set_ui(num1, 0); mpz_set_ui(num2, 1);
    mpz_set_ui(den1, 1); mpz_set_ui(den2, 0);

    for (int i = 0; i < len; i++) {
        mpz_init(convs[i].k);
        mpz_init(convs[i].d);

        // num = a * num2 + num1
        mpz_mul(tmp, cf[i], num2);
        mpz_add(num, tmp, num1);
        mpz_set(convs[i].k, num);

        // den = a * den2 + den1
        mpz_mul(tmp, cf[i], den2);
        mpz_add(den, tmp, den1);
        mpz_set(convs[i].d, den);

        mpz_set(num1, num2);
        mpz_set(num2, num);
        mpz_set(den1, den2);
        mpz_set(den2, den);
    }

    mpz_clears(num1, num2, den1, den2, num, den, tmp, NULL);
}

// Check if convergent gives valid d by recovering phi(N)
int try_recover_d(mpz_t e, mpz_t N, Fraction frac) {
    if (mpz_cmp_ui(frac.k, 0) == 0) return 0;

    mpz_t ed_minus_1, phi, rem;
    mpz_inits(ed_minus_1, phi, rem, NULL);

    mpz_mul(ed_minus_1, e, frac.d);
    mpz_sub_ui(ed_minus_1, ed_minus_1, 1);

    mpz_mod(rem, ed_minus_1, frac.k);
    if (mpz_cmp_ui(rem, 0) != 0) {
        mpz_clears(ed_minus_1, phi, rem, NULL);
        return 0;
    }

    mpz_divexact(phi, ed_minus_1, frac.k);

    // Compute discriminant of x^2 - (N - phi + 1)x + N = 0
    mpz_t s, discr, sqrt_d, temp;
    mpz_inits(s, discr, sqrt_d, temp, NULL);

    mpz_sub(s, N, phi);     // s = N - phi
    mpz_add_ui(s, s, 1);    // s = N - phi + 1

    mpz_mul(discr, s, s);           // discr = s^2
    mpz_mul_ui(temp, N, 4);         // temp = 4N
    mpz_sub(discr, discr, temp);    // discr = s^2 - 4N

    if (mpz_sgn(discr) < 0) {
        mpz_clears(ed_minus_1, phi, rem, s, discr, sqrt_d, temp, NULL);
        return 0;
    }

    mpz_sqrt(sqrt_d, discr);
    mpz_mul(temp, sqrt_d, sqrt_d);
    if (mpz_cmp(temp, discr) != 0) {
        mpz_clears(ed_minus_1, phi, rem, s, discr, sqrt_d, temp, NULL);
        return 0; // Not a perfect square
    }

    // p = (s + sqrt(d)) / 2
    mpz_add(temp, s, sqrt_d);
    if (mpz_even_p(temp)) {
        mpz_divexact_ui(temp, temp, 2);

        mpz_t q;
        mpz_init(q);
        mpz_divexact(q, N, temp);

        gmp_printf("\n[+] Private key recovered!\n");
        gmp_printf("    d = %Zd\n", frac.d);
        gmp_printf("    p = %Zd\n", temp);
        gmp_printf("    q = %Zd\n", q);

        mpz_clear(q);
        mpz_clears(ed_minus_1, phi, rem, s, discr, sqrt_d, temp, NULL);
        return 1;
    }

    mpz_clears(ed_minus_1, phi, rem, s, discr, sqrt_d, temp, NULL);
    return 0;
}

int main() {
    mpz_t e, N;
    mpz_inits(e, N, NULL);

    // Replace with vulnerable RSA key values
    mpz_set_str(e, "17993", 10);
    mpz_set_str(N, "90581", 10);

    // GMP can handle large values like:
    // mpz_set_str(e, "65537", 10);
    // mpz_set_str(N, "<your_large_modulus>", 10);

    printf("Attempting Wiener's attack...\n");

    const int MAX_TERMS = 1000;
    mpz_t cf[MAX_TERMS];
    Fraction convs[MAX_TERMS];

    int len = continued_fraction(e, N, cf, MAX_TERMS);
    generate_convergents(cf, len, convs);

    for (int i = 0; i < len; i++) {
        if (try_recover_d(e, N, convs[i])) {
            return 0; // Success
        }
    }

    printf("[-] Attack failed. No weak d found.\n");
    return 0;
}
