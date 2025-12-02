#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MAX_LEN 1024
#define MILLER_RABIN_ITERATIONS 8

// Modular exponentiation
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

// Miller-Rabin Primality Test
int is_prime(uint64_t n, int k) 
{
    if (n < 2) return 0;
    if (n == 2 || n == 3) return 1;
    if (n % 2 == 0) return 0;

    uint64_t d = n - 1;
    int r = 0;
    while ((d & 1) == 0) 
    {
        d >>= 1;
        r++;
    }

    for (int i = 0; i < k; i++) 
    {
        uint64_t a = 2 + rand() % (n - 4);
        uint64_t x = modexp(a, d, n);
        if (x == 1 || x == n - 1) continue;

        int is_composite = 1;
        for (int j = 0; j < r - 1; j++) 
        {
            x = modexp(x, 2, n);
            if (x == n - 1) 
            {
                is_composite = 0;
                break;
            }
        }
        if (is_composite)
            return 0;
    }
    return 1;
}

// GCD
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

// Modular inverse (Extended Euclidean Algorithm)
int64_t modinv(int64_t a, int64_t m) 
{
    int64_t m0 = m, t, q;
    int64_t x0 = 0, x1 = 1;
    if (m == 1) return 0;
    while (a > 1) 
    {
        q = a / m;
        t = m;
        m = a % m; a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }
    return (x1 < 0) ? x1 + m0 : x1;
}

// Generate a large prime in a range
uint64_t generate_large_prime(uint64_t min, uint64_t max) 
{
    uint64_t p;
    do 
    {
        p = min + (uint64_t)rand() * rand() % (max - min);
    } while (!is_prime(p, MILLER_RABIN_ITERATIONS));
    return p;
}

int main() 
{
    srand(time(NULL));

    char mode[16];
    printf("Enter mode (encrypt/decrypt): ");
    scanf("%s", mode);
    getchar(); // consume leftover newline

    if (strcmp(mode, "encrypt") == 0) 
    {
        // Generate two large primes (~10–15 digits)
        uint64_t p = generate_large_prime(1000000000ULL, 100000000000ULL);  // ~10-12 digits
        uint64_t q = generate_large_prime(1000000000ULL, 100000000000ULL);
        uint64_t n = p * q;
        uint64_t phi = (p - 1) * (q - 1);

        uint64_t e;
        do 
        {
            e = 65537;  // Common choice
        } while (gcd(e, phi) != 1);

        uint64_t d = modinv(e, phi);

        printf("\nGenerated RSA Parameters:\n");
        printf("p = %llu\nq = %llu\nn = %llu\nphi(n) = %llu\ne = %llu\nd = %llu\n",
               p, q, n, phi, e, d);

        char plaintext[MAX_LEN];
        printf("\nEnter plaintext: ");
        fgets(plaintext, MAX_LEN, stdin);
        plaintext[strcspn(plaintext, "\n")] = '\0';

        size_t len = strlen(plaintext);
        uint64_t ciphertext[MAX_LEN];

        printf("\nCiphertext:\n");
        for (size_t i = 0; i < len; i++) 
        {
            ciphertext[i] = modexp((uint64_t)plaintext[i], e, n);
            printf("%llu ", ciphertext[i]);
        }
        printf("\n");

    } else if (strcmp(mode, "decrypt") == 0) 
    {
        uint64_t n, d;
        printf("Enter modulus n: ");
        scanf("%llu", &n);
        printf("Enter private exponent d: ");
        scanf("%llu", &d);
        getchar(); // consume newline

        printf("Enter ciphertext (space-separated, end with -1):\n");

        uint64_t ciphertext[MAX_LEN];
        size_t len = 0;
        while (1) 
        {
            uint64_t val;
            if (scanf("%llu", &val) != 1 || val == (uint64_t)-1)
                break;
            ciphertext[len++] = val;
        }

        char decrypted[MAX_LEN];
        for (size_t i = 0; i < len; i++) 
        {
            decrypted[i] = (char)modexp(ciphertext[i], d, n);
        }
        decrypted[len] = '\0';

        printf("\nDecrypted text:\n%s\n", decrypted);

    } else {
        printf("Invalid mode. Use 'encrypt' or 'decrypt'.\n");
    }

    return 0;
}
