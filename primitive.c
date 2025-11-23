#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// Function to compute gcd of two numbers
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Function to compute Euler's totient function
int euler_totient(int n) {
    int result = 1;
    for (int i = 2; i < n; i++) {
        if (gcd(i, n) == 1) {
            result++;
        }
    }
    return result;
}

// Function to check if a number is a generator of Z*_n
bool is_generator(int a, int n) {
    if (gcd(a, n) != 1) return false;
    
    int phi = euler_totient(n);
    int order = phi;
    
    // Check divisors of phi
    for (int d = 2; d <= phi; d++) {
        if (phi % d == 0) {
            // Compute a^d mod n
            int power = 1;
            for (int i = 0; i < d; i++) {
                power = (power * a) % n;
            }
            if (power == 1) {
                order = d;
                break;
            }
        }
    }
    
    return order == phi;
}

// Function to find a generator of Z*_n (returns 0 if none found)
int find_generator(int n) {
    int phi = euler_totient(n);
    for (int a = 2; a < n; a++) {
        if (is_generator(a, n)) {
            return a;
        }
    }
    return 0;
}

int main()
 {
    printf("Numbers n ≤ 100 where a generator of Z*_n is not a generator of Z*_{n²}:\n");
    
    for (int n = 2; n <= 100; n++) 
         {
            if (n == 2 || n == 4) {
            // These cases are cyclic
        } else {
            // Check if n is a power of an odd prime
            bool is_power_of_odd_prime = false;
            for (int p = 3; p <= n; p += 2) {
                if (gcd(p, n) == p) {  // p divides n
                    int temp = n;
                    while (temp % p == 0) {
                        temp /= p;
                    }
                    if (temp == 1) {
                        is_power_of_odd_prime = true;
                        break;
                    }
                }
            }
            
            // Check if n is 2 times a power of an odd prime
            bool is_2_times_power_of_odd_prime = false;
            if (n % 2 == 0) {
                int temp = n / 2;
                if (temp > 1) {
                    for (int p = 3; p <= temp; p += 2) {
                        if (gcd(p, temp) == p) {  // p divides temp
                            while (temp % p == 0) {
                                temp /= p;
                            }
                            if (temp == 1) {
                                is_2_times_power_of_odd_prime = true;
                                break;
                            }
                        }
                    }
                }
            }
            
            if (!(n == 2 || n == 4 || is_power_of_odd_prime || is_2_times_power_of_odd_prime)) {
                continue;  // Z*_n is not cyclic, skip
            }
        }
        
        int g = find_generator(n);
        if (g == 0) continue;  // no generator found
        
        int n_squared = n * n;
        if (!is_generator(g, n_squared)) {
            printf("n = %d (example generator: %d)\n", n, g);
        }
    }
    
    return 0;
}