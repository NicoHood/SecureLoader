
#ifndef __AES_256_CBC_H__
#define __AES_256_CBC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "aes.h"

// DEFINES
#define AES256_CBC_LENGTH     16

// USEFUL functions
static inline void aesXorVectors(uint8_t *dest, const uint8_t *src, uint8_t nbytes);
static inline void aes256CbcEncrypt(aes256_ctx_t *ctx, uint8_t *data, uint16_t dataLen);
static inline void aes256CbcDecrypt(aes256_ctx_t *ctx, uint8_t *data, uint16_t dataLen) __attribute__((always_inline)); //TODO use? -> bootloader authentification



/*!    \fn     void aesXorVectors(uint8_t* dest, uint8_t* src, uint8_t nbytes)
*    \brief    Do xor between dest and src and save it inside dest
*
*   \param  dest - destination of xor
*   \param  src - source of xor data
*   \param  nbytes - number of bytes to be xored between dest and src
*/
static inline void aesXorVectors(uint8_t *dest, const uint8_t *src, uint8_t nbytes)
{
    while (nbytes--)
    {
        *dest ^= *src;
        dest++;
        src++;
    }
}


void aes256CbcEncrypt(aes256_ctx_t *ctx, uint8_t *data, uint16_t dataLen)
{
    // Encrypt the data
    for (uint16_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // Start from the beginning to XOR next data
        uint8_t* pos = data + i;

        // XOR plaintext with previous data (assuming the IV is prepended!)
        aesXorVectors(pos, pos - AES256_CBC_LENGTH, AES256_CBC_LENGTH);

        // Encrypt next block
        aes256_enc(pos, ctx);
    }
}

// data points to the IV
// Assumes IV is prepended!
// TODO assumed the IV is set to zero
// Does not touch the IV!
// dataLen excludes the IV
void aes256CbcDecrypt(aes256_ctx_t *ctx, uint8_t *data, uint16_t dataLen)
{
    // Decrypt the data
    for (uint16_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // Start from the end to not overwrite previous data
        uint8_t* pos = data + dataLen - i;

        // Decrypt next block
        aes256_dec(pos, ctx);

        // XOR cbcMac with previous data (assuming the IV is prepended!)
        aesXorVectors(pos, pos - AES256_CBC_LENGTH, AES256_CBC_LENGTH);
    }
}



static inline void aes256CbcMacCalculate(aes256_ctx_t* ctx, uint8_t *data, const uint16_t dataLen)
{
    // Check if dataLen is a multiple of AES256_CBC_LENGTH
    if (dataLen % AES256_CBC_LENGTH != 0)
    {
        return;
    }

    // CBC-MAC is appended to the data. DataLen excludes the CBC-MAC.
    uint8_t* cbcMac = data + dataLen;

    // Set IV to Zero
    memset(cbcMac, 0x00, AES256_CBC_LENGTH);

    // Loop will update cbcMac for each block
    for (uint16_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // XOR cbcMac with data
        aesXorVectors(cbcMac, data + i, AES256_CBC_LENGTH);

        // Encrypt next block
        aes256_enc(cbcMac, ctx);
    }
}

//     Compare if CBC-MAC matches with the input data
// TODO note The input cbcMac will be overwritten for recalculating the IV
// TODO assume cbc mac is appended to the data
// TODO For a forward compare just recalculate the CBC-MAC and compare the result
static inline bool aes256CbcMacReverseCompare(aes256_context *ctx, uint8_t *data, const uint16_t dataLen)
{
    // Check if dataLen is a multiple of AES256_CBC_LENGTH
    if (dataLen % AES256_CBC_LENGTH != 0)
    {
        return true;
    }

    // CBC-MAC is appended to the data. DataLen excludes the CBC-MAC.
    uint8_t* cbcMac = data + dataLen;

    // Loop will update cbcMac for each block
    for (uint16_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // Decrypt next block
        aes256_dec(cbcMac, ctx);

        // XOR cbcMac with data
        aesXorVectors(cbcMac, data + dataLen - i - AES256_CBC_LENGTH, AES256_CBC_LENGTH);
    }

    // Check if CBC-MAC matches, run the full loop to avoid timing attacks
    bool error = false;
    for (uint8_t i = 0; i < AES256_CBC_LENGTH; i++)
    {
        if (cbcMac[i] != 0x00)
        {
            error = true;
        }
    }
    return error;
}

#ifdef __cplusplus
}
#endif

#endif
