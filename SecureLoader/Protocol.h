
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#if defined(__AVR__)
#include <avr/io.h>
#elif !defined(SPM_PAGESIZE)
#error Please define SPM_PAGESIZE before you include this file!
#endif

// Data to programm a flash page that was sent by the host
typedef union
{
    uint8_t raw[0];
    struct
    {
        union
        {
            uint16_t PageAddress;
            uint8_t padding[AES256_CBC_LENGTH];
        };
        union
        {
            uint16_t PageDataWords[SPM_PAGESIZE/2];
            uint8_t PageDataBytes[SPM_PAGESIZE];
        };
        uint8_t cbcMac[AES256_CBC_LENGTH];
    };
} ProgrammFlashPage_t;

// Set a flash page address, that can be requested by the host afterwards
typedef union
{
    uint8_t raw[0];
    struct
    {
        uint16_t PageAddress;
    };
} SetFlashPage_t;

// Data to read a flash page that was requested by the host
typedef union
{
    uint8_t raw[0];
    struct
    {
        uint16_t PageAddress;
        union
        {
            uint16_t PageDataWords[SPM_PAGESIZE/2];
            uint8_t PageDataBytes[SPM_PAGESIZE];
        };
    };
} ReadFlashPage_t;

// Data to simpler calculate the new Bootloader Key, IV prepended
typedef union
{
    uint8_t raw[0];
    struct
    {
        uint8_t IV[AES256_CBC_LENGTH];

        // Data package from the PC to change the Bootloader Key
        union
        {
            uint8_t raw[0];
            struct
            {
                uint8_t BootloaderKey[32];
                uint8_t cbcMac[AES256_CBC_LENGTH];
            };
        } data;
    };
} newBootloaderKey_t;

// Data to authenticate the Bootloader to the PC, IV prepended
typedef union
{
    uint8_t raw[0];
    struct
    {
        uint8_t IV[AES256_CBC_LENGTH];

        // Data package from the PC with a signed encrypted challenge
        union
        {
            uint8_t raw[0];
            struct
            {
                uint8_t challenge[AES256_CBC_LENGTH];
                uint8_t cbcMac[AES256_CBC_LENGTH];
            };
        } data;
    };
} authenticateBootloader_t;

#ifdef __cplusplus
}
#endif

#endif
