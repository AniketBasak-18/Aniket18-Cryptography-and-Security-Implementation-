/*
 * aes_ecb.c
 *
 * AES-128 implementation (encryption & decryption) in ECB mode with PKCS#7 padding.
 * - Key expansion
 * - SubBytes / ShiftRows / MixColumns / AddRoundKey
 * - Single-block encrypt / decrypt
 * - ECB mode for multiple blocks with PKCS#7 padding
 *
 * Compile: gcc -O2 -std=c11 aes_ecb.c -o aes_ecb
 * Run: ./aes_ecb
 *
 * Test vector included:
 * Plain:  3243f6a8885a308d313198a2e0370734
 * Key:    2b7e151628aed2a6abf7158809cf4f3c
 * Cipher: 3925841d02dc09fbdc118597196a0b32
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* AES constants */
static const uint8_t sbox[256] = {
    /* 0x00 */ 0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    /* 0x10 */ 0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    /* 0x20 */ 0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    /* 0x30 */ 0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    /* 0x40 */ 0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    /* 0x50 */ 0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    /* 0x60 */ 0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    /* 0x70 */ 0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    /* 0x80 */ 0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    /* 0x90 */ 0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    /* 0xa0 */ 0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    /* 0xb0 */ 0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    /* 0xc0 */ 0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    /* 0xd0 */ 0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    /* 0xe0 */ 0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    /* 0xf0 */ 0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t inv_sbox[256] = {
    /* 0x00 */ 0x52,0x09,0x6A,0xD5,0x30,0x36,0xA5,0x38,0xBF,0x40,0xA3,0x9E,0x81,0xF3,0xD7,0xFB,
    /* 0x10 */ 0x7C,0xE3,0x39,0x82,0x9B,0x2F,0xFF,0x87,0x34,0x8E,0x43,0x44,0xC4,0xDE,0xE9,0xCB,
    /* 0x20 */ 0x54,0x7B,0x94,0x32,0xA6,0xC2,0x23,0x3D,0xEE,0x4C,0x95,0x0B,0x42,0xFA,0xC3,0x4E,
    /* 0x30 */ 0x08,0x2E,0xA1,0x66,0x28,0xD9,0x24,0xB2,0x76,0x5B,0xA2,0x49,0x6D,0x8B,0xD1,0x25,
    /* 0x40 */ 0x72,0xF8,0xF6,0x64,0x86,0x68,0x98,0x16,0xD4,0xA4,0x5C,0xCC,0x5D,0x65,0xB6,0x92,
    /* 0x50 */ 0x6C,0x70,0x48,0x50,0xFD,0xED,0xB9,0xDA,0x5E,0x15,0x46,0x57,0xA7,0x8D,0x9D,0x84,
    /* 0x60 */ 0x90,0xD8,0xAB,0x00,0x8C,0xBC,0xD3,0x0A,0xF7,0xE4,0x58,0x05,0xB8,0xB3,0x45,0x06,
    /* 0x70 */ 0xD0,0x2C,0x1E,0x8F,0xCA,0x3F,0x0F,0x02,0xC1,0xAF,0xBD,0x03,0x01,0x13,0x8A,0x6B,
    /* 0x80 */ 0x3A,0x91,0x11,0x41,0x4F,0x67,0xDC,0xEA,0x97,0xF2,0xCF,0xCE,0xF0,0xB4,0xE6,0x73,
    /* 0x90 */ 0x96,0xAC,0x74,0x22,0xE7,0xAD,0x35,0x85,0xE2,0xF9,0x37,0xE8,0x1C,0x75,0xDF,0x6E,
    /* 0xa0 */ 0x47,0xF1,0x1A,0x71,0x1D,0x29,0xC5,0x89,0x6F,0xB7,0x62,0x0E,0xAA,0x18,0xBE,0x1B,
    /* 0xb0 */ 0xFC,0x56,0x3E,0x4B,0xC6,0xD2,0x79,0x20,0x9A,0xDB,0xC0,0xFE,0x78,0xCD,0x5A,0xF4,
    /* 0xc0 */ 0x1F,0xDD,0xA8,0x33,0x88,0x07,0xC7,0x31,0xB1,0x12,0x10,0x59,0x27,0x80,0xEC,0x5F,
    /* 0xd0 */ 0x60,0x51,0x7F,0xA9,0x19,0xB5,0x4A,0x0D,0x2D,0xE5,0x7A,0x9F,0x93,0xC9,0x9C,0xEF,
    /* 0xe0 */ 0xA0,0xE0,0x3B,0x4D,0xAE,0x2A,0xF5,0xB0,0xC8,0xEB,0xBB,0x3C,0x83,0x53,0x99,0x61,
    /* 0xf0 */ 0x17,0x2B,0x04,0x7E,0xBA,0x77,0xD6,0x26,0xE1,0x69,0x14,0x63,0x55,0x21,0x0C,0x7D
};

static const uint8_t Rcon[11] = {
    0x00, /* unused 0 */
    0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36
};

/* Helper: xtime (multiply by x i.e. 0x02) */
static inline uint8_t xtime(uint8_t x) {
    return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00));
}

/* Galois field multiplication */
static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t res = 0;
    while (b) {
        if (b & 1) res ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return res;
}

/* State representation: state[row][col] with 0<=row,col<4
 * Input bytes are mapped column-wise:
 * state[row][col] = in[col*4 + row]
 */

/* SubBytes */
static void SubBytes(uint8_t state[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            state[r][c] = sbox[state[r][c]];
}

/* InvSubBytes */
static void InvSubBytes(uint8_t state[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            state[r][c] = inv_sbox[state[r][c]];
}

/* ShiftRows */
static void ShiftRows(uint8_t state[4][4]) {
    uint8_t tmp[4];
    // row 1: left rotate 1
    tmp[0]=state[1][0]; tmp[1]=state[1][1]; tmp[2]=state[1][2]; tmp[3]=state[1][3];
    state[1][0]=tmp[1]; state[1][1]=tmp[2]; state[1][2]=tmp[3]; state[1][3]=tmp[0];

    // row 2: left rotate 2
    tmp[0]=state[2][0]; tmp[1]=state[2][1]; tmp[2]=state[2][2]; tmp[3]=state[2][3];
    state[2][0]=tmp[2]; state[2][1]=tmp[3]; state[2][2]=tmp[0]; state[2][3]=tmp[1];

    // row 3: left rotate 3 (or right rotate 1)
    tmp[0]=state[3][0]; tmp[1]=state[3][1]; tmp[2]=state[3][2]; tmp[3]=state[3][3];
    state[3][0]=tmp[3]; state[3][1]=tmp[0]; state[3][2]=tmp[1]; state[3][3]=tmp[2];
}

/* InvShiftRows */
static void InvShiftRows(uint8_t state[4][4]) {
    uint8_t tmp[4];
    // row1: right rotate 1
    tmp[0]=state[1][0]; tmp[1]=state[1][1]; tmp[2]=state[1][2]; tmp[3]=state[1][3];
    state[1][0]=tmp[3]; state[1][1]=tmp[0]; state[1][2]=tmp[1]; state[1][3]=tmp[2];

    // row2: right rotate 2
    tmp[0]=state[2][0]; tmp[1]=state[2][1]; tmp[2]=state[2][2]; tmp[3]=state[2][3];
    state[2][0]=tmp[2]; state[2][1]=tmp[3]; state[2][2]=tmp[0]; state[2][3]=tmp[1];

    // row3: right rotate 3 (left rotate 1)
    tmp[0]=state[3][0]; tmp[1]=state[3][1]; tmp[2]=state[3][2]; tmp[3]=state[3][3];
    state[3][0]=tmp[1]; state[3][1]=tmp[2]; state[3][2]=tmp[3]; state[3][3]=tmp[0];
}

/* MixColumns */
static void MixColumns(uint8_t state[4][4]) {
    for (int c = 0; c < 4; ++c) {
        uint8_t a0 = state[0][c], a1 = state[1][c], a2 = state[2][c], a3 = state[3][c];
        uint8_t t = a0 ^ a1 ^ a2 ^ a3;
        uint8_t u = a0;
        state[0][c] = a0 ^ t ^ xtime(a0 ^ a1);
        state[1][c] = a1 ^ t ^ xtime(a1 ^ a2);
        state[2][c] = a2 ^ t ^ xtime(a2 ^ a3);
        state[3][c] = a3 ^ t ^ xtime(a3 ^ a0);
    }
}

/* InvMixColumns */
static void InvMixColumns(uint8_t state[4][4]) {
    for (int c = 0; c < 4; ++c) {
        uint8_t a0 = state[0][c], a1 = state[1][c], a2 = state[2][c], a3 = state[3][c];
        state[0][c] = (uint8_t)(gmul(a0,0x0e) ^ gmul(a1,0x0b) ^ gmul(a2,0x0d) ^ gmul(a3,0x09));
        state[1][c] = (uint8_t)(gmul(a0,0x09) ^ gmul(a1,0x0e) ^ gmul(a2,0x0b) ^ gmul(a3,0x0d));
        state[2][c] = (uint8_t)(gmul(a0,0x0d) ^ gmul(a1,0x09) ^ gmul(a2,0x0e) ^ gmul(a3,0x0b));
        state[3][c] = (uint8_t)(gmul(a0,0x0b) ^ gmul(a1,0x0d) ^ gmul(a2,0x09) ^ gmul(a3,0x0e));
    }
}

/* AddRoundKey: roundKey is 16 bytes */
static void AddRoundKey(uint8_t state[4][4], const uint8_t *roundKey) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            state[r][c] ^= roundKey[c*4 + r];
}

/* Key expansion for AES-128
 * key: 16 bytes input
 * roundKeys: must be 176 bytes (11*16)
 */
static void KeyExpansion(const uint8_t key[16], uint8_t roundKeys[176]) {
    // First round key is the original key
    memcpy(roundKeys, key, 16);
    uint8_t temp[4];
    int bytesGenerated = 16;
    int rconIter = 1;
    while (bytesGenerated < 176) {
        // last 4 bytes
        for (int i = 0; i < 4; ++i)
            temp[i] = roundKeys[bytesGenerated - 4 + i];

        if (bytesGenerated % 16 == 0) {
            // rotate
            uint8_t t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;
            // subword
            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];
            // Rcon
            temp[0] ^= Rcon[rconIter];
            rconIter++;
        }
        // XOR with word [bytesGenerated - 16]
        for (int i = 0; i < 4; ++i) {
            roundKeys[bytesGenerated] = roundKeys[bytesGenerated - 16] ^ temp[i];
            bytesGenerated++;
        }
    }
}

/* Convert 16-byte input array to state */
static void bytes_to_state(const uint8_t in[16], uint8_t state[4][4]) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            state[r][c] = in[c*4 + r];
}

/* Convert state to 16-byte output array */
static void state_to_bytes(uint8_t state[4][4], uint8_t out[16]) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            out[c*4 + r] = state[r][c];
}

/* Encrypt one 128-bit block (16 bytes) */
static void AES128_EncryptBlock(const uint8_t in[16], const uint8_t roundKeys[176], uint8_t out[16]) {
    uint8_t state[4][4];
    bytes_to_state(in, state);

    AddRoundKey(state, roundKeys); // round 0

    for (int round = 1; round <= 9; ++round) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, roundKeys + round*16);
    }

    // final round
    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, roundKeys + 10*16);

    state_to_bytes(state, out);
}

/* Decrypt one 128-bit block (16 bytes) */
static void AES128_DecryptBlock(const uint8_t in[16], const uint8_t roundKeys[176], uint8_t out[16]) {
    uint8_t state[4][4];
    bytes_to_state(in, state);

    AddRoundKey(state, roundKeys + 10*16); // initial with last round key

    for (int round = 9; round >= 1; --round) {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey(state, roundKeys + round*16);
        InvMixColumns(state);
    }

    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(state, roundKeys); // final add round key (round 0)

    state_to_bytes(state, out);
}

/* Hex helpers */
static uint8_t hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

static void hexstr_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    size_t hexlen = strlen(hex);
    size_t expect = out_len * 2;
    if (hexlen < expect) {
        // left-pad with zeros if shorter
        size_t pad = expect - hexlen;
        for (size_t i = 0; i < pad; ++i) {
            out[i/2] = 0;
        }
    }
    for (size_t i = 0; i < out_len; ++i) {
        size_t ix = i*2;
        out[i] = (hex_nibble(hex[ix]) << 4) | hex_nibble(hex[ix+1]);
    }
}

/* safer hex convert that tolerates arbitrary length hex (but expects exact 2*out_len chars) */
static int hex_to_bytes_exact(const char *hex, uint8_t *out, size_t out_len) {
    size_t hexlen = strlen(hex);
    if (hexlen != out_len*2) return 0;
    for (size_t i = 0; i < out_len; ++i) {
        out[i] = (hex_nibble(hex[2*i]) << 4) | hex_nibble(hex[2*i+1]);
    }
    return 1;
}

static void bytes_to_hex(const uint8_t *in, size_t len, char *out) {
    static const char hexchars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; ++i) {
        out[i*2] = hexchars[(in[i] >> 4) & 0xF];
        out[i*2+1] = hexchars[in[i] & 0xF];
    }
    out[len*2] = '\0';
}

/* ECB mode - PKCS#7 padding for encryption */
static uint8_t *AES128_ECB_Encrypt(const uint8_t *plaintext, size_t plaintext_len, const uint8_t key[16], size_t *out_len) {
    // build round keys
    uint8_t roundKeys[176];
    KeyExpansion(key, roundKeys);

    // padding
    size_t block_size = 16;
    size_t pad_len = block_size - (plaintext_len % block_size);
    if (pad_len == 0) pad_len = block_size;
    size_t total_len = plaintext_len + pad_len;
    uint8_t *buf = malloc(total_len);
    if (!buf) return NULL;
    memcpy(buf, plaintext, plaintext_len);
    // PKCS#7 padding bytes all equal to pad_len
    memset(buf + plaintext_len, (uint8_t)pad_len, pad_len);

    // encrypt in place to output buffer
    uint8_t *out = malloc(total_len);
    if (!out) { free(buf); return NULL; }

    uint8_t inblock[16], outblock[16];
    for (size_t offset = 0; offset < total_len; offset += 16) {
        memcpy(inblock, buf + offset, 16);
        AES128_EncryptBlock(inblock, roundKeys, outblock);
        memcpy(out + offset, outblock, 16);
    }

    free(buf);
    *out_len = total_len;
    return out;
}

/* ECB decrypt (removes PKCS#7 padding) */
static uint8_t *AES128_ECB_Decrypt(const uint8_t *ciphertext, size_t ciphertext_len, const uint8_t key[16], size_t *out_len) {
    if (ciphertext_len % 16 != 0) return NULL;
    uint8_t roundKeys[176];
    KeyExpansion(key, roundKeys);

    uint8_t *buf = malloc(ciphertext_len);
    if (!buf) return NULL;

    uint8_t inblock[16], outblock[16];
    for (size_t offset = 0; offset < ciphertext_len; offset += 16) {
        memcpy(inblock, ciphertext + offset, 16);
        AES128_DecryptBlock(inblock, roundKeys, outblock);
        memcpy(buf + offset, outblock, 16);
    }

    // remove PKCS#7 padding
    if (ciphertext_len == 0) { free(buf); return NULL; }
    uint8_t pad = buf[ciphertext_len - 1];
    if (pad < 1 || pad > 16) {
        // invalid padding
        free(buf);
        return NULL;
    }
    // verify padding bytes
    for (size_t i = 0; i < pad; ++i) {
        if (buf[ciphertext_len - 1 - i] != pad) {
            free(buf);
            return NULL;
        }
    }
    size_t plain_len = ciphertext_len - pad;
    uint8_t *out = malloc(plain_len + 1);
    if (!out) { free(buf); return NULL; }
    memcpy(out, buf, plain_len);
    out[plain_len] = 0;
    free(buf);
    *out_len = plain_len;
    return out;
}

/* Test driver */
int main(void) {
    // Provided test vector
    const char *pt_hex = "3243f6a8885a308d313198a2e0370734";
    const char *key_hex = "2b7e151628aed2a6abf7158809cf4f3c";
    const char *expected_cipher_hex = "3925841d02dc09fbdc118597196a0b32";

    uint8_t plaintext[16], key[16], out[16], expected_cipher[16];
    if (!hex_to_bytes_exact(pt_hex, plaintext, 16)) {
        printf("Plaintext hex length incorrect\n"); return 1;
    }
    if (!hex_to_bytes_exact(key_hex, key, 16)) {
        printf("Key hex length incorrect\n"); return 1;
    }
    if (!hex_to_bytes_exact(expected_cipher_hex, expected_cipher, 16)) {
        printf("Expected cipher hex length incorrect\n"); return 1;
    }

    uint8_t roundKeys[176];
    KeyExpansion(key, roundKeys);

    AES128_EncryptBlock(plaintext, roundKeys, out);

    char out_hex[33];
    bytes_to_hex(out, 16, out_hex);
    printf("Computed ciphertext: %s\n", out_hex);
    printf("Expected ciphertext: %s\n", expected_cipher_hex);
    if (memcmp(out, expected_cipher, 16) == 0) {
        printf("Single-block AES-128 encryption test: OK ✅\n");
    } else {
        printf("Single-block AES-128 encryption test: FAILED ❌\n");
    }

    // Also test decryption of the computed ciphertext
    uint8_t decrypted[16];
    AES128_DecryptBlock(out, roundKeys, decrypted);
    char dec_hex[33];
    bytes_to_hex(decrypted, 16, dec_hex);
    printf("Decrypted back: %s\n", dec_hex);

    // Demonstrate ECB mode with padding
    const char *multi_plain = "This is a test of AES-128 ECB mode. It will use PKCS#7 padding!";
    size_t multi_plain_len = strlen(multi_plain);
    size_t enc_len;
    uint8_t *enc = AES128_ECB_Encrypt((const uint8_t*)multi_plain, multi_plain_len, key, &enc_len);
    if (!enc) { fprintf(stderr, "ECB encrypt failed\n"); return 1; }
    char *enc_hex = malloc(enc_len*2 + 1);
    bytes_to_hex(enc, enc_len, enc_hex);
    printf("\nECB encrypted (hex, %zu bytes):\n%s\n", enc_len, enc_hex);

    size_t dec_len;
    uint8_t *dec = AES128_ECB_Decrypt(enc, enc_len, key, &dec_len);
    if (!dec) {
        fprintf(stderr, "ECB decrypt failed (bad padding?)\n");
    } else {
        printf("ECB decrypted (%zu bytes):\n%.*s\n", dec_len, (int)dec_len, dec);
        free(dec);
    }

    free(enc_hex);
    free(enc);

    return 0;
}
