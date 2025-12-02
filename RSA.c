#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Miller-Rabin Primality Test (modular exponentiation part)
uint64_t modexp(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1)
            result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

// Miller-Rabin test
int is_prime(uint64_t n, int k) {
    if (n < 2) return 0;
    if (n == 2 || n == 3) return 1;
    if (n % 2 == 0) return 0;

    // Write n - 1 as 2^r * d
    uint64_t d = n - 1;
    int r = 0;
    while ((d & 1) == 0) {
        d >>= 1;
        r++;
    }

    for (int i = 0; i < k; i++) {
        uint64_t a = 2 + rand() % (n - 4);
        uint64_t x = modexp(a, d, n);

        if (x == 1 || x == n - 1)
            continue;

        int is_composite = 1;
        for (int j = 0; j < r - 1; j++) {
            x = modexp(x, 2, n);
            if (x == n - 1) {
                is_composite = 0;
                break;
            }
        }

        if (is_composite)
            return 0; // Composite
    }
    return 1; // Probably prime
}

// GCD
uint64_t gcd(uint64_t a, uint64_t b) {
    while (b) {
        uint64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// Extended Euclidean Algorithm to find modular inverse
int64_t modinv(int64_t a, int64_t m) {
    int64_t m0 = m, t, q;
    int64_t x0 = 0, x1 = 1;

    if (m == 1)
        return 0;

    while (a > 1) {
        q = a / m;
        t = m;

        m = a % m, a = t;
        t = x0;

        x0 = x1 - q * x0;
        x1 = t;
    }

    if (x1 < 0)
        x1 += m0;

    return x1;
}

// Generate a random prime in range [min, max]
uint64_t generate_prime(int min, int max) {
    uint64_t p;
    do {
        p = min + rand() % (max - min);
    } while (!is_prime(p, 5));
    return p;
}

int main() {
    srand(time(NULL));

    // Step 1: Generate two primes
    uint64_t p = generate_prime(100, 300);
    uint64_t q = generate_prime(100, 300);
    printf("Chosen primes:\np = %llu\nq = %llu\n", p, q);

    // Step 2: Compute n and phi
    uint64_t n = p * q;
    uint64_t phi = (p - 1) * (q - 1);
    printf("n = p*q = %llu\n", n);
    printf("Euler's Totient (phi) = %llu\n", phi);

    // Step 3: Choose e such that gcd(e, phi) = 1
    uint64_t e;
    do {
        e = 3 + rand() % (phi - 3); // small odd number
    } while (gcd(e, phi) != 1);
    printf("Chosen e = %llu (public exponent)\n", e);

    // Step 4: Compute d ≡ e⁻¹ mod phi
    uint64_t d = modinv(e, phi);
    printf("Computed d = %llu (private exponent)\n", d);

    // Step 5: Encrypt and Decrypt a message
    uint64_t plaintext;
    printf("\nEnter a number as plaintext (less than %llu): ", n);
    scanf("%llu", &plaintext);

    uint64_t ciphertext = modexp(plaintext, e, n);
    printf("Encrypted ciphertext = %llu\n", ciphertext);

    uint64_t decrypted = modexp(ciphertext, d, n);
    printf("Decrypted plaintext = %llu\n", decrypted);

    return 0;
}
