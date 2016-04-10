
#ifndef __AES_256_CBC_H__
#define __AES_256_CBC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "aes.h"

// USEFUL functions
static inline void aesXorVectors(uint8_t *dest, const uint8_t *src, uint8_t nbytes);

// CBC functions
static inline void aes256CbcEncrypt(aes256_ctx_t *ctx, uint8_t *data, const size_t dataLen);
static inline void aes256CbcDecrypt(aes256_ctx_t *ctx, uint8_t *data, const size_t dataLen) __attribute__((always_inline)); //TODO use?

// CBC-MAC functions
static inline void aes256CbcMacCalculate(aes256_ctx_t* ctx, uint8_t *data, const size_t dataLen);
static inline bool aes256CbcMacReverseCompare(aes256_context* ctx, uint8_t* data, const size_t dataLen);

// DEFINES
#define AES256_CBC_LENGTH     16

/*! \fn     void aesXorVectors(uint8_t* dest, uint8_t* src, uint8_t nbytes)
*   \brief  Do xor between dest and src and save it inside dest
*
*   \param  dest - destination of xor
*   \param  src - source of xor data
*   \param  nbytes - number of bytes to be xored between dest and src
*/
void aesXorVectors(uint8_t *dest, const uint8_t *src, uint8_t nbytes)
{
    while (nbytes--)
    {
        *dest ^= *src;
        dest++;
        src++;
    }
}

/*! \fn     void aes256CbcEncrypt(aes256_ctx_t* ctx, uint8_t* data, size_t dataLen)
*   \brief  Encrypt data and save it in data.
*   \note   Reserved space for the IV has to be prepended to the data.
*
*   \param  ctx - context
*   \param  data - pointer to data, starting with the IV, this is also the location to store encrypted data
*   \param  dataLen - size of data (excluding the IV)
*/
void aes256CbcEncrypt(aes256_ctx_t* ctx, uint8_t* data, const size_t dataLen)
{
    // Check if dataLen is a multiple of AES256_CBC_LENGTH
    if (dataLen % AES256_CBC_LENGTH != 0)
    {
        return;
    }

    // Set IV to Zero
    memset(data, 0x00, AES256_CBC_LENGTH);

    // Encrypt the data
    for (size_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // Get next block
        data += AES256_CBC_LENGTH;

        // XOR plaintext with previous data (assuming the IV is prepended!)
        aesXorVectors(data, data - AES256_CBC_LENGTH, AES256_CBC_LENGTH);

        // Encrypt next block
        aes256_enc(data, ctx);
    }
}

/*! \fn     void aes256CbcDecrypt(aes256_ctx_t* ctx, uint8_t* data, size_t dataLen)
*   \brief  Decrypt data and save it in data.
*   \note   Reserved space for the IV has to be prepended to the data.
*
*   \param  ctx - context
*   \param  data - pointer to data, starting with the IV, this is also the location to store decrypted data
*   \param  dataLen - size of data (excluding the IV)
*/
void aes256CbcDecrypt(aes256_ctx_t* ctx, uint8_t* data, const size_t dataLen)
{
    // Check if dataLen is a multiple of AES256_CBC_LENGTH
    if (dataLen % AES256_CBC_LENGTH != 0)
    {
        return;
    }

    // Set IV to Zero
    memset(data, 0x00, AES256_CBC_LENGTH);

    // Start from the end to not overwrite previous data (skip IV)
    data += dataLen;

    // Decrypt the data
    for (size_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // Decrypt next block
        aes256_dec(data, ctx);

        // XOR cbcMac with previous data (assuming the IV is prepended!)
        aesXorVectors(data, data - AES256_CBC_LENGTH, AES256_CBC_LENGTH);

        // Get next block
        data -= AES256_CBC_LENGTH;
    }
}

/*! \fn     void aes256CbcMacCalculate(aes256_ctx_t* ctx, uint8_t* data, const size_t dataLen)
*   \brief  Calculate CBC-MAC and save it at the end of data.
*   \note   Reserved space for the CBC-MAC has to be appended to the data.
*
*   \param  ctx - context
*   \param  data - pointer to data, this is also the location to store the CBC-MAC at the end
*   \param  dataLen - size of data (excluding the CBC-MAC)
*/
void aes256CbcMacCalculate(aes256_ctx_t* ctx, uint8_t* data, const size_t dataLen)
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
    for (size_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
    {
        // XOR cbcMac with data
        aesXorVectors(cbcMac, data + i, AES256_CBC_LENGTH);

        // Encrypt next block
        aes256_enc(cbcMac, ctx);
    }
}

/*! \fn     void aes256CbcMacCalculate(aes256_ctx_t* ctx, uint8_t* data, const size_t dataLen)
*   \brief  Reverse calculate CBC-MAC and check if IV matches.
*   \note   The CBC-MAC has to be appended to the data and will be overwritten with the IV.
*
*   \param  ctx - context
*   \param  data - pointer to data, this is also the location to store the CBC-MAC at the end
*   \param  dataLen - size of data (excluding the CBC-MAC)
*   \return CBC-MAC comparison error has occured
*/
bool aes256CbcMacReverseCompare(aes256_context* ctx, uint8_t* data, const size_t dataLen)
{
    // Check if dataLen is a multiple of AES256_CBC_LENGTH
    if (dataLen % AES256_CBC_LENGTH != 0)
    {
        return true;
    }

    // CBC-MAC is appended to the data. DataLen excludes the CBC-MAC.
    uint8_t* cbcMac = data + dataLen;

    // Loop will update cbcMac for each block
    for (size_t i = 0; i < dataLen; i += AES256_CBC_LENGTH)
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
