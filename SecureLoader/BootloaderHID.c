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
 *  Main source file for the HID class bootloader. This file contains the complete bootloader logic.
 */

#include "BootloaderHID.h"

/** Flag to indicate if the bootloader should be running, or should exit and allow the application code to run
 *  via a soft reset. When cleared, the bootloader will abort, the USB interface will shut down and the application
 *  started via a forced watchdog reset.
 */
static bool RunBootloader = true;

/** Magic lock for forced application start. If the HWBE fuse is programmed and BOOTRST is unprogrammed, the bootloader
 *  will start if the /HWB line of the AVR is held low and the system is reset. However, if the /HWB line is still held
 *  low when the application attempts to start via a watchdog reset, the bootloader will re-start. If set to the value
 *  \ref MAGIC_BOOT_KEY the special init function \ref Application_Jump_Check() will force the application to start.
 */
uint16_t MagicBootKey ATTR_NO_INIT;

#define sizeof_member(type, member) sizeof(((type *)0)->member)

// Data to programm a flash page that was sent by the host
typedef union{
	uint8_t raw[0];
	struct{
		union{
			uint16_t PageAddress;
			uint8_t padding[AES256_CBC_LENGTH];
		};
		uint16_t PageData[SPM_PAGESIZE/2];
		uint8_t cbcMac[AES256_CBC_LENGTH];
	};
} ProgrammFlashPage_t;

// Set a flash page address, that can be requested by the host afterwards
typedef union{
	uint8_t raw[0];
	struct{
		uint16_t PageAddress;
	};
} SetFlashPage_t;

// Data to read a flash page that was requested by the host
typedef union{
	uint8_t raw[0];
	struct{
		uint16_t PageAddress;
		uint16_t PageData[SPM_PAGESIZE/2];
		uint8_t cbcMac[AES256_CBC_LENGTH]; // TODO with CBC MAC? If yes add padding
	};
} ReadFlashPage_t;

// Data to change the Bootloader Key
typedef union{
	uint8_t raw[0];
	struct{
		uint8_t BootloaderKey[32];
		uint8_t cbcMac[AES256_CBC_LENGTH];
	};
} changeBootloaderKey_t;

// Temporary USB_Buffer holds data to send/receive data.
// It gets overwritten with every request.
// Since more features use the same buffer you can
// set an PageAddress via several methods (SetFlashPage or ProgrammFlashPage).
static uint8_t USB_Buffer[MAX(MAX(sizeof(ReadFlashPage_t), sizeof(ProgrammFlashPage_t)),
													sizeof(changeBootloaderKey_t))];

// Bootloader Key TODO store in eeprom/progmem
static uint8_t BootloaderKey[32] = {
	0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
	0x2b,	0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
	0x1f, 0x35, 0x2c, 0x07, 0x3b,	0x61, 0x08, 0xd7,
	0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

// AES256 CBC-MAC context variable
static aes256CbcMacCtx_t ctx;

/** Special startup routine to check if the bootloader was started via a watchdog reset, and if the magic application
 *  start key has been loaded into \ref MagicBootKey. If the bootloader started via the watchdog and the key is valid,
 *  this will force the user application to start via a software jump.
 */
void Application_Jump_Check(void)
{
	/* Turn off the watchdog */
	uint8_t mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();

	/* If the reset source was the bootloader and the key is correct, clear it and jump to the application */
	if (((mcusr_mirror & (1 << WDRF)) && (MagicBootKey == MAGIC_BOOT_KEY)) || (mcusr_mirror & (1 << PORF)))
	{
		/* Clear the boot key and jump to the user application */
		MagicBootKey = 0;

		// TODO check if sketch is present?? required?

		// cppcheck-suppress constStatement
		((void (*)(void))0x0000)();
	}
}



/** Main program entry point. This routine configures the hardware required by the bootloader, then continuously
 *  runs the bootloader processing routine until instructed to soft-exit.
 */
int main(void)
{
	/* Setup hardware required for the bootloader */
	SetupHardware();

	/* Enable global interrupts so that the USB stack can function */
	GlobalInterruptEnable();

	//uart_putchars("\r\nStartup\r\n-----------------------------------------\r\n");

  // Simpler version of USB_USBTask()
	// TODO use interrupt as alternative
	do{
		// TODO why required?
		Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

		if (Endpoint_IsSETUPReceived())
		  USB_Device_ProcessControlRequest();
	} while (RunBootloader);

	/* Disconnect from the host - USB interface will be reset later along with the AVR */
	USB_Detach();

	/* Unlock the forced application start mode of the bootloader if it is restarted */
	MagicBootKey = MAGIC_BOOT_KEY;

	/* Enable the watchdog and force a timeout to reset the AVR */
	wdt_enable(WDTO_250MS);

	for (;;);
}

/** Configures all hardware required for the bootloader. */
static void SetupHardware(void)
{
	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Relocate the interrupt vector table to the bootloader section */
	MCUCR = (1 << IVCE);
	MCUCR = (1 << IVSEL);

	// TODO remove debug serial
	//uart_init();

	/* Initialize USB subsystem */
	USB_Init();
}

#include "inline.h"
