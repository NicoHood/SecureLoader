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

#include "BootloaderAPI.h"

uint8_t BootloaderAPI_ReadByte(const address_size_t address)
{
    // Read PGM from any location of the flash
    return pgm_read_byte_auto(address);
}

bool BootloaderAPI_EraseFillWritePage(const address_size_t address, const uint16_t* words)
{
    // Do not write out of bounds
    if ((address & (SPM_PAGESIZE - 1)) || (address > (FLASHEND - SPM_PAGESIZE)))
    {
        return true;
    }

    // Erase the given FLASH page, ready to be programmed
    boot_page_erase(address);
    boot_spm_busy_wait();

    // Write each of the FLASH page's bytes in sequence
    uint8_t PageWord;
    for (PageWord = 0; PageWord < (SPM_PAGESIZE / 2); PageWord++)
    {
        // Write the next data word to the FLASH page
        boot_page_fill(address + ((uint16_t)PageWord << 1), *words);
        words++;
    }

    // Write the filled FLASH page to memory
    boot_page_write(address);
    boot_spm_busy_wait();

    // Re-enable RWW section
    boot_rww_enable();

    // No error occured
    return false;
}
