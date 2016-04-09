/*
             LUFA Library
     Copyright (C) Dean Camera, 2016.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2016  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#ifndef _BOOTLOADER_API_H_
#define _BOOTLOADER_API_H_

    /* Includes: */
        #include <avr/io.h>
        #include <avr/boot.h>
        #include <avr/pgmspace.h>
        #include <stdbool.h>

    /* Enable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            extern "C" {
        #endif

    /* Macros: */
        #if (FLASHEND > USHRT_MAX)
            typedef uint32_t address_size_t;
            #define pgm_read_byte_auto(address) pgm_read_byte_far(address)
            #define getPageAddress(address) ((uint32_t)address << 8)
            #define setPageAddress(address) ((uint16_t)(address >> 8))
        #else
            typedef uint16_t address_size_t;
            #define pgm_read_byte_auto(address) pgm_read_byte_near(address)
            #define getPageAddress(address) (address)
            #define setPageAddress(address) (address)
        #endif

    /* Function Prototypes: */
        bool BootloaderAPI_EraseFillWritePage(const address_size_t address, const uint16_t* words) __attribute__ ((used, section (".apitable_functions")));
        uint8_t BootloaderAPI_ReadByte(const address_size_t address) __attribute__ ((used, section (".apitable_functions")));
        static inline bool BootloaderAPI_ReadPage(const address_size_t Address, uint8_t* data);
        static inline void BootloaderAPI_WriteEEPROM(uint8_t* data, void* Address, uint8_t length);
        static inline void BootloaderAPI_UpdateEEPROM(uint8_t* data, void* Address, uint8_t length);

    /* Inline Functions: */
        bool BootloaderAPI_ReadPage(const address_size_t Address, uint8_t* data)
        {
            // Do not read out of bounds
            if (Address & (SPM_PAGESIZE - 1) || (Address > FLASHEND)) {
                return true;
            }

            for(uint8_t i = 0; i < SPM_PAGESIZE; i++){
                *data = pgm_read_byte_auto(Address + i);
                data++;
            }

            return false;
        }

        void BootloaderAPI_WriteEEPROM(uint8_t* data, void* Address, uint8_t length)
        {
            // Write data (max 8 bit length) and wait for eeprom to finish
            for(uint8_t i = 0; i < length; i++){
                eeprom_write_byte(Address + i, *((uint8_t*)data + i));
            }
            eeprom_busy_wait();
        }

        void BootloaderAPI_UpdateEEPROM(uint8_t* data, void* Address, uint8_t length)
        {
            // Write data (max 8 bit length) and wait for eeprom to finish
            for(uint8_t i = 0; i < length; i++){
                eeprom_update_byte(Address + i, *((uint8_t*)(data + i)));
            }
            eeprom_busy_wait();
        }

    /* Disable C linkage for C++ Compilers: */
        #if defined(__cplusplus)
            }
        #endif

#endif
