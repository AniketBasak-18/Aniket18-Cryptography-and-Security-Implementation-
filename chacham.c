#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Rotate left 32-bit
uint32_t rotate_left(uint32_t value, int bits) 
{
    return (value << bits) | (value >> (32 - bits));
}

// Quarter Round function
void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *a += *b; *d ^= *a; *d = rotate_left(*d, 16);
    *c += *d; *b ^= *c; *b = rotate_left(*b, 12);
    *a += *b; *d ^= *a; *d = rotate_left(*d, 8);
    *c += *d; *b ^= *c; *b = rotate_left(*b, 7);
}

// One ChaCha20 block (generates 64 bytes of keystream)
void chacha20_block(uint32_t output[16], const uint32_t input[16]) {
    uint32_t state[16];
    memcpy(state, input, sizeof(state));

    for (int i = 0; i < 10; i++) 
    {
        // Column rounds
        quarter_round(&state[0], &state[4], &state[8], &state[12]);
        quarter_round(&state[1], &state[5], &state[9], &state[13]);
        quarter_round(&state[2], &state[6], &state[10], &state[14]);
        quarter_round(&state[3], &state[7], &state[11], &state[15]);

        // Diagonal rounds
        quarter_round(&state[0], &state[5], &state[10], &state[15]);
        quarter_round(&state[1], &state[6], &state[11], &state[12]);
        quarter_round(&state[2], &state[7], &state[8], &state[13]);
        quarter_round(&state[3], &state[4], &state[9], &state[14]);
    }

    for (int i = 0; i < 16; i++) 
    {
        output[i] = state[i] + input[i]; // Final addition step
    }
}

// Read 4 bytes as little-endian 32-bit integer
uint32_t read_le32(const uint8_t *bytes) 
{
    return ((uint32_t)bytes[0]) |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

// Write 32-bit integer to 4 bytes (little-endian)
void write_le32(uint8_t *bytes, uint32_t value) 
{
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
    bytes[2] = (value >> 16) & 0xff;
    bytes[3] = (value >> 24) & 0xff;
}

// ChaCha20 encryption/decryption
void chacha20_encrypt(
    uint8_t *output,
    const uint8_t *input,
    size_t length,
    const uint8_t key[32],
    const uint8_t nonce[12],
    uint32_t counter
) 
{
    uint32_t state[16];
    uint32_t block[16];
    uint8_t keystream[64];

    // Constants
    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;

    // Load 256-bit key (8 x 32-bit words)
    for (int i = 0; i < 8; i++)
        state[4 + i] = read_le32(&key[i * 4]);

    // Counter and nonce
    state[12] = counter;
    state[13] = read_le32(&nonce[0]);
    state[14] = read_le32(&nonce[4]);
    state[15] = read_le32(&nonce[8]);

    for (size_t i = 0; i < length; i += 64) 
    {
        chacha20_block(block, state);

        // Convert keystream block to bytes
        for (int j = 0; j < 16; j++)
            write_le32(&keystream[j * 4], block[j]);

        // XOR with plaintext/ciphertext
        size_t block_len = (length - i < 64) ? (length - i) : 64;
        for (size_t j = 0; j < block_len; j++)
            output[i + j] = input[i + j] ^ keystream[j];

        state[12]++; // Increment counter
    }
}

// MAIN FUNCTION
int main() {
    uint8_t key[32] = {0};     // All-zero key for simplicity
    uint8_t nonce[12] = {0};   // All-zero nonce
    char message[1024];
    uint8_t result[1024];
    int mode;

    printf("=== ChaCha20 Cipher ===\n");
    printf("1. Encrypt\n2. Decrypt\nChoose mode: ");
    scanf("%d", &mode);
    getchar(); // consume newline

    printf("Enter message: ");
    fgets(message, sizeof(message), stdin);

    size_t msg_len = strlen(message);
    if (message[msg_len - 1] == '\n') message[--msg_len] = '\0';

    chacha20_encrypt(result, (uint8_t *)message, msg_len, key, nonce, 0);

    if (mode == 1) {
        printf("\nEncrypted (hex): ");
        for (size_t i = 0; i < msg_len; i++)
            printf("%02x", result[i]);
        printf("\n");
    } else {
        printf("\nDecrypted: ");
        for (size_t i = 0; i < msg_len; i++)
            printf("%c", result[i]);
        printf("\n");
    }

    return 0;
}
