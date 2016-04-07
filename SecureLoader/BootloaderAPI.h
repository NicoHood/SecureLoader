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

/** \file
 *
 *  Header file for BootloaderAPI.c.
 */

#ifndef _BOOTLOADER_API_H_
#define _BOOTLOADER_API_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/boot.h>
		#include <avr/pgmspace.h>
		#include <stdbool.h>

		//#include <LUFA/Common/Common.h>

		#if (FLASHEND > USHRT_MAX)
			typedef uint32_t address_size_t;
			#define pgm_read_byte_auto(address) pgm_read_byte_far(address)
			#define getPageAddress(address) ((uint32_t)address << 8)
		#else
			typedef uint16_t address_size_t;
			#define pgm_read_byte_auto(address) pgm_read_byte_near(address)
			#define getPageAddress(address) (address)
		#endif

	/* Function Prototypes: */
		void BootloaderAPI_Test(const address_size_t Address, const uint16_t* Words) __attribute__ ((used, section (".apitable_functions")));
		void    BootloaderAPI_ErasePage(const address_size_t Address);
		void    BootloaderAPI_WritePage(const address_size_t Address);
		void    BootloaderAPI_FillWord(const address_size_t Address, const uint16_t Word);
		static inline bool    BootloaderAPI_EraseFillWritePage(const address_size_t Address, const uint16_t* Words);
		uint8_t BootloaderAPI_ReadSignature(const uint16_t Address);
		uint8_t BootloaderAPI_ReadFuse(const uint16_t Address);
		uint8_t BootloaderAPI_ReadLock(void);
		void    BootloaderAPI_WriteLock(const uint8_t LockBits);

		static inline bool BootloaderAPI_EraseFillWritePage(const address_size_t Address, const uint16_t* Words)
		{
			// TODO only write data if its new, to preserv flash destruction on replay attacks
			// add check inside loop
			// move erase down (works)
			//hexdump(&Address, 2);

			// Do not write out of bounds TODO FLASHEND
			if (Address & (SPM_PAGESIZE - 1)) {
				return true;
			}

			/* Erase the given FLASH page, ready to be programmed */
			boot_page_erase(Address);
			boot_spm_busy_wait();

			/* Write each of the FLASH page's bytes in sequence */
			uint8_t PageWord;
			for (PageWord = 0; PageWord < (SPM_PAGESIZE / 2); PageWord++)
			{
				/* Write the next data word to the FLASH page */
				boot_page_fill(Address + ((uint16_t)PageWord << 1), *Words);
				Words++;
			}

			/* Write the filled FLASH page to memory */
			boot_page_write(Address);
			boot_spm_busy_wait();

			/* Re-enable RWW section */
			boot_rww_enable();

			return false;
		}

		static inline bool BootloaderAPI_ReadPage(const address_size_t Address, uint8_t* data)
		{
			// Do not read out of bounds TODO FLASHEND
			if (Address & (SPM_PAGESIZE - 1)) {
				return true;
			}

			for(uint8_t i = 0; i < SPM_PAGESIZE; i++){
				*data = pgm_read_byte_auto(Address + i);
				data++;
			}

			return false;
		}

		static inline void BootloaderAPI_WriteEEPROM(uint8_t* data, void* Address, uint8_t length)
		{
			// Write data (max 8 bit length) and wait for eeprom to finish
			for(uint8_t i = 0; i < length; i++){
				eeprom_write_byte(Address + i, *((uint8_t*)data + i));
			}
			eeprom_busy_wait();
		}

		static inline void BootloaderAPI_UpdateEEPROM(uint8_t* data, void* Address, uint8_t length)
		{
			// Write data (max 8 bit length) and wait for eeprom to finish
			for(uint8_t i = 0; i < length; i++){
				eeprom_update_byte(Address + i, *((uint8_t*)(data + i)));
			}
			eeprom_busy_wait();
		}

#endif
