#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h> // __rdtsc
#include <gmp.h>       // GMP

#define MAX_LEN 1024

uint64_t modexp(uint64_t base, uint64_t exp, uint64_t mod)
{
    uint64_t result = 1;
    base %= mod;
    while (exp) 
    {
        if (exp & 1)
            result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

uint64_t gcd(uint64_t a, uint64_t b)
{
    while (b) 
    {
        uint64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int64_t modinv(int64_t a, int64_t m)
{
    int64_t m0 = m, t, q;
    int64_t x0 = 0, x1 = 1;
    if (m == 1) return 0;
    while (a > 1) 
    {
        q = a / m;
        t = m;
        m = a % m;
        a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }
    return (x1 < 0) ? x1 + m0 : x1;
}

// Generate 512-bit prime using GMP
void generate_512bit_prime(mpz_t prime)
{
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_randseed_ui(state, time(NULL));

    mpz_urandomb(prime, state, 512); // generate 512-bit random number
    mpz_nextprime(prime, prime);    // get next prime
    gmp_randclear(state);
}

int main()
{
    srand(time(NULL));
    char mode[16];
    printf("Enter mode (encrypt/decrypt): ");
    scanf("%s", mode);
    getchar();

    if (strcmp(mode, "encrypt") == 0)
    {
        // GMP variables
        mpz_t p, q, n, phi, e, d, p1, q1;
        mpz_inits(p, q, n, phi, e, d, p1, q1, NULL);

        // Generate 512-bit primes
        uint64_t start_gen = __rdtsc();
        generate_512bit_prime(p);
        generate_512bit_prime(q);

        mpz_mul(n, p, q);

        mpz_sub_ui(p1, p, 1);
        mpz_sub_ui(q1, q, 1);
        mpz_mul(phi, p1, q1);

        mpz_set_ui(e, 65537);
        while (1)
        {
            mpz_t g;
            mpz_init(g);
            mpz_gcd(g, e, phi);
            if (mpz_cmp_ui(g, 1) == 0)
            {
                mpz_clear(g);
                break;
            }
            mpz_add_ui(e, e, 2);
            mpz_clear(g);
        }

        mpz_invert(d, e, phi);
        uint64_t end_gen = __rdtsc();

        printf("\nGenerated RSA Parameters:\n");
        gmp_printf("p = %Zd\nq = %Zd\nn = %Zd\nphi(n) = %Zd\ne = %Zd\nd = %Zd\n", p, q, n, phi, e, d);
        printf("Clock cycles for key generation: %llu\n", end_gen - start_gen);

        // Input plaintext
        char plaintext[MAX_LEN];
        printf("\nEnter plaintext: ");
        fgets(plaintext, MAX_LEN, stdin);
        plaintext[strcspn(plaintext, "\n")] = '\0';

        size_t len = strlen(plaintext);
        mpz_t m, c;
        mpz_inits(m, c, NULL);
        uint64_t start_enc = __rdtsc();

        printf("\nCiphertext:\n");
        for (size_t i = 0; i < len; i++)
        {
            mpz_set_ui(m, (unsigned char)plaintext[i]);
            mpz_powm(c, m, e, n);
            gmp_printf("%Zd ", c);
        }
        uint64_t end_enc = __rdtsc();

        printf("\nClock cycles for encryption: %llu\n", end_enc - start_enc);
        printf("Average cycles per byte: %.2f\n", (double)(end_enc - start_enc) / len);

        mpz_clears(p, q, n, phi, e, d, p1, q1, m, c, NULL);
    }
    else if (strcmp(mode, "decrypt") == 0)
    {
        mpz_t n, d, c, m;
        mpz_inits(n, d, c, m, NULL);

        printf("Enter modulus n: ");
        gmp_scanf("%Zd", n);
        printf("Enter private exponent d: ");
        gmp_scanf("%Zd", d);
        getchar();

        printf("Enter ciphertext (space-separated, end with -1):\n");
        char line[8192];
        fgets(line, sizeof(line), stdin);
        char *token = strtok(line, " ");
        uint64_t start_dec = __rdtsc();

        printf("\nDecrypted text:\n");
        while (token && strcmp(token, "-1") != 0)
        {
            mpz_set_str(c, token, 10);
            mpz_powm(m, c, d, n);
            printf("%c", (char)mpz_get_ui(m));
            token = strtok(NULL, " ");
        }

        uint64_t end_dec = __rdtsc();
        printf("\nClock cycles for decryption: %llu\n", end_dec - start_dec);

        mpz_clears(n, d, c, m, NULL);
    }
    else
    {
        printf("Invalid mode. Use 'encrypt' or 'decrypt'.\n");
    }

    return 0;
}
