#ifndef _SHA256_HPP
#define _SHA256_HPP

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#define SHA256_BLOCK_SIZE 32 // SHA256 outputs a 32 uint8_t digest

typedef struct
{
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
} SHA256_CTX;


/**
 * Initialiaze SHA256 hash context
 * @param ctx SHA256 hash context
 */
void sha256_init(SHA256_CTX *ctx);

/**
 * Updates SHA256 with new part of data
 * @param ctx SHA256 hash context
 * @param data Incoming data
 * @param len Incoming data len
 */
void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len);

/**
 * Calculate output value of sha256
 * @param ctx Sha256 context
 * @param hash Outgoing hash
 */
void sha256_final(SHA256_CTX *ctx, uint8_t hash[]);

#endif