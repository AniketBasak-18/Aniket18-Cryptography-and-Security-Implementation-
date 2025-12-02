
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <gmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <errno.h>


#define PRIME_BITS 512            /* set to 1024 as requested */
#define ITER_PRIME_GEN 1000000        /* default 1000; set to 1000000 for assignment - WARNING: slow */
#define PROB_PRIME_REPS 25         /* Miller-Rabin reps for mpz_probab_prime_p */
#define MSG_BITS (PRIME_BITS*2)    /* message bits: choose 2*prime for security; set to 1024 if you want exact */
const char *PRNG_NAME = "Mersenne Twister (gmp_randinit_mt)";

/* public exponent e = 2^16 + 1 */
const unsigned long E_EXP = (1UL<<16) + 1UL;

static inline uint64_t rdtsc_start(void) {
    unsigned int a, d;
    asm volatile("cpuid\n\t"  /* serialize */
                 "rdtsc\n\t"
                 "mov %%eax, %0\n\t"
                 "mov %%edx, %1\n\t"
                 : "=r"(a), "=r"(d)
                 :
                 : "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)d << 32) | a;
}
static inline uint64_t rdtsc_end(void) {
    unsigned int a, d;
    asm volatile("rdtscp\n\t"   /* rdtscp is ordered */
                 "mov %%eax, %0\n\t"
                 "mov %%edx, %1\n\t"
                 "cpuid\n\t"      /* serialize */
                 : "=r"(a), "=r"(d)
                 :
                 : "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)d << 32) | a;
}

/* pin to CPU 0 for consistent measurements */
void pin_to_cpu0(void) {
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    if (sched_setaffinity(0, sizeof(cpus), &cpus) != 0) {
        perror("sched_setaffinity");
    }
}

/* seed gmp RNG from /dev/urandom */
void seed_gmp_rng(gmp_randstate_t state) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/urandom");
        exit(1);
    }
    unsigned long seed;
    ssize_t r = read(fd, &seed, sizeof(seed));
    close(fd);
    if (r != sizeof(seed)) {
        fprintf(stderr, "Could not fully read seed\n");
        exit(1);
    }
    gmp_randseed_ui(state, seed);
}

/* Generate random odd candidate of 'bits' bits with MSB and LSB set to 1 */
void gen_candidate_with_msb_lsb(mpz_t out, gmp_randstate_t st, unsigned int bits) {
    mpz_urandomb(out, st, bits);        /* uniform random in [0,2^bits) */
    mpz_setbit(out, bits - 1);          /* set MSB */
    mpz_setbit(out, 0);                 /* set LSB to 1 => odd */
}

/* generate a prime with given bit length; uses mpz_probab_prime_p */
void generate_prime_with_checks(mpz_t prime, gmp_randstate_t st, unsigned int bits) {
    int is_prime = 0;
    while (!is_prime) {
        gen_candidate_with_msb_lsb(prime, st, bits);
        /* mpz_probab_prime_p returns:
           0 = composite, 1 = probably prime, 2 = definitely prime
           Using PROB_PRIME_REPS for Miller-Rabin repetitions via mpz_probab_prime_p's internal param */
        int res = mpz_probab_prime_p(prime, PROB_PRIME_REPS);
        if (res > 0) is_prime = 1;
    }
}

/* print mpz as hex with bit-size note */
void print_mpz_info(const char *label, mpz_t v) {
    gmp_printf("%s (%zu bits): %Zx\n", label, mpz_sizeinbase(v, 2), v);
}

/* safe multiplication with timing */
uint64_t timed_mul(mpz_t out, mpz_t a, mpz_t b) {
    uint64_t s = rdtsc_start();
    mpz_mul(out, a, b);
    uint64_t e = rdtsc_end();
    return e - s;
}

/* safe subtraction / phi compute timing */
uint64_t timed_phi(mpz_t phi, mpz_t p, mpz_t q) {
    /* phi = (p-1)*(q-1) */
    mpz_t t1, t2;
    mpz_init(t1);
    mpz_init(t2);
    uint64_t s = rdtsc_start();
    mpz_sub_ui(t1, p, 1);
    mpz_sub_ui(t2, q, 1);
    mpz_mul(phi, t1, t2);
    uint64_t e = rdtsc_end();
    mpz_clear(t1);
    mpz_clear(t2);
    return e - s;
}

/* modular inverse timing */
uint64_t timed_modinv(mpz_t d, mpz_t e, mpz_t phi) {
    uint64_t s = rdtsc_start();
    if (mpz_invert(d, e, phi) == 0) {
        /* no inverse - shouldn't happen for chosen e if phi odd and gcd=1 */
        uint64_t e_t = rdtsc_end();
        return e_t - s;
    }
    uint64_t e_t = rdtsc_end();
    return e_t - s;
}

/* modular exponentiation timing */
uint64_t timed_powmod(mpz_t out, mpz_t base, mpz_t exp, mpz_t mod) {
    uint64_t s = rdtsc_start();
    mpz_powm(out, base, exp, mod);
    uint64_t e = rdtsc_end();
    return e - s;
}

int main(int argc, char **argv) {
    /* optional args: [prime_bits] [iter_prime_gen] */
    unsigned int bits = PRIME_BITS;
    unsigned long iter = ITER_PRIME_GEN;
    if (argc >= 2) bits = (unsigned int)atoi(argv[1]);
    if (argc >= 3) iter = (unsigned long)atol(argv[2]);

    printf("Assignment steps 1-4 implementation (following RSA_Assignment.pdf). PRNG: %s\n", PRNG_NAME);
    printf("Prime bits: %u; ITER_PRIME_GEN: %lu\n", bits, iter);
    printf("Ensure CPU governor=performance and pin to single core for best reproducibility.\n\n");

    /* pin to CPU0 */
    pin_to_cpu0();

    /* lock memory to avoid paging interference */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("mlockall (warning)");
    }

    /* init GMP RNG */
    gmp_randstate_t st;
    gmp_randinit_mt(st);
    seed_gmp_rng(st);

    /* containers */
    mpz_t p, q, N, phi, e_mpz, d;
    mpz_inits(p, q, N, phi, e_mpz, d, NULL);
    mpz_set_ui(e_mpz, E_EXP);

    /* statistics for prime generation cycles */
    uint64_t min_cycles_p = (uint64_t)-1, max_cycles_p = 0;
    __uint128_t sum_cycles_p = 0; /* may be large: use 128-bit accumulator */
    uint64_t min_cycles_q = (uint64_t)-1, max_cycles_q = 0;
    __uint128_t sum_cycles_q = 0;

    /* a single iter will do a pair p and q generation and record cycles for each */
    for (unsigned long i = 0; i < iter; ++i) {
        /* generate p */
        uint64_t s1 = rdtsc_start();
        generate_prime_with_checks(p, st, bits);
        uint64_t e1 = rdtsc_end();
        uint64_t cyc_p = e1 - s1;
        if (cyc_p < min_cycles_p) min_cycles_p = cyc_p;
        if (cyc_p > max_cycles_p) max_cycles_p = cyc_p;
        sum_cycles_p += cyc_p;

        /* generate q */
        uint64_t s2 = rdtsc_start();
        generate_prime_with_checks(q, st, bits);
        uint64_t e2 = rdtsc_end();
        uint64_t cyc_q = e2 - s2;
        if (cyc_q < min_cycles_q) min_cycles_q = cyc_q;
        if (cyc_q > max_cycles_q) max_cycles_q = cyc_q;
        sum_cycles_q += cyc_q;

        /* optional progress for long runs */
        if ((i+1) % (iter/10 == 0 ? 1 : (iter/10)) == 0) {
            printf("Progress: %lu / %lu iterations\n", i+1, iter);
        }
    }

    /* compute min/max/avg for p and q prime generation */
    double avg_p = (double)sum_cycles_p / (double)iter;
    double avg_q = (double)sum_cycles_q / (double)iter;

    printf("\n=== Prime generation statistics (each prime separately) ===\n");
    printf("PRNG: %s\n", PRNG_NAME);
    printf("p generation cycles: min=%" PRIu64 ", max=%" PRIu64 ", avg=%.2f\n", min_cycles_p, max_cycles_p, avg_p);
    printf("q generation cycles: min=%" PRIu64 ", max=%" PRIu64 ", avg=%.2f\n", min_cycles_q, max_cycles_q, avg_q);
    printf("Detailed calculation (p): sum=%lu, avg = sum/iter -> %.2f\n", (unsigned long)sum_cycles_p, avg_p);
    printf("Detailed calculation (q): sum=%lu, avg = sum/iter -> %.2f\n", (unsigned long)sum_cycles_q, avg_q);

    /* Now for steps 2-4, do one fresh generation to measure N, phi, d, encryption/decryption timings */
    generate_prime_with_checks(p, st, bits);
    generate_prime_with_checks(q, st, bits);

    /* Step 2: compute N and phi */
    uint64_t cyc_N = timed_mul(N, p, q);
    uint64_t cyc_phi = timed_phi(phi, p, q);
    printf("\nStep 2: N and phi calculation cycles:\n");
    printf("N = p * q cycles = %" PRIu64 "\n", cyc_N);
    printf("phi = (p-1)*(q-1) cycles = %" PRIu64 "\n", cyc_phi);
    printf("Detail: (timed_mul used rdtsc before/after mpz_mul), (timed_phi subtracts then multiplies)\n");

    /* Step 3: compute d = e^-1 mod phi */
    uint64_t cyc_d = timed_modinv(d, e_mpz, phi);
    printf("\nStep 3: private key computation cycles:\n");
    printf("e = %lu, cycles to compute d = e^-1 mod phi : %" PRIu64 "\n", E_EXP, cyc_d);

    /* Step 4: message, encrypt, decrypt */
    mpz_t m, c, mprime;
    mpz_inits(m, c, mprime, NULL);

    /* choose random message m of MSG_BITS with MSB = 0 (assignment says 1024 bits with MSB=0) */
    /* We'll create a random message of exactly MSG_BITS bits, then clear MSB */
    mpz_urandomb(m, st, MSG_BITS);
    mpz_clrbit(m, MSG_BITS - 1); /* MSB = 0 */
    /* Ensure m < N */
    if (mpz_cmp(m, N) >= 0) {
        mpz_mod(m, m, N);
    }

    /* encryption c = m^e mod N */
    uint64_t cyc_enc = timed_powmod(c, m, e_mpz, N);
    /* decryption m' = c^d mod N */
    uint64_t cyc_dec = timed_powmod(mprime, c, d, N);

    printf("\nStep 4: encryption / decryption cycles:\n");
    printf("encryption (c = m^e mod N): %" PRIu64 " cycles\n", cyc_enc);
    printf("decryption (m' = c^d mod N): %" PRIu64 " cycles\n", cyc_dec);

    /* verify */
    if (mpz_cmp(m, mprime) == 0) {
        printf("Verification: m' == m : OK\n");
    } else {
        printf("Verification: m' != m : FAILURE\n");
    }

    /* Print final computed values briefly */
    print_mpz_info("p", p);
    print_mpz_info("q", q);
    print_mpz_info("N", N);
    print_mpz_info("phi", phi);

    /* cleanup */
    mpz_clears(p, q, N, phi, e_mpz, d, m, c, mprime, NULL);
    gmp_randclear(st);

    return 0;
}
