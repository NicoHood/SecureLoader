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

static uint8_t mcusr_mirror ATTR_NO_INIT;

// Data to programm a flash page that was sent by the host
static union{
	uint8_t raw[0];
	struct{
		union{
			uint16_t PageAddress;
			uint8_t padding[AES256_CBC_LENGTH];
		};
		uint16_t PageData[SPM_PAGESIZE/2];
		uint8_t cbcMac[AES256_CBC_LENGTH];
	};
} ProgrammFlashPage;

// Data to read a flash page that was requested by the host
static union{
	uint8_t raw[0];
	struct{
		uint16_t PageAddress;
		uint16_t PageData[SPM_PAGESIZE/2];
		uint8_t cbcMac[AES256_CBC_LENGTH]; // TODO with CBC MAC? If yes add padding
	};
} ReadFlashPage;

// Data to change the Bootloader Key
static union{
	uint8_t raw[0];
	struct{
		uint8_t BootloaderKey[32];
		uint8_t cbcMac[AES256_CBC_LENGTH];
	};
} changeBootloaderKey;

// Bootloader Key TODO store in progmem
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
	mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();

	/* If the reset source was the bootloader and the key is correct, clear it and jump to the application */
	if ((mcusr_mirror & (1 << WDRF)) && (MagicBootKey == MAGIC_BOOT_KEY) || (mcusr_mirror & (1 << PORF)))
	{
		/* Clear the boot key and jump to the user application */
		MagicBootKey = 0;

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

	while (RunBootloader)
	  USB_USBTask();

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

/** Event handler for the USB_ConfigurationChanged event. This configures the device's endpoints ready
 *  to relay data to and from the attached USB host.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	/* Setup HID Report Endpoint */
	Endpoint_ConfigureEndpoint(HID_IN_EPADDR, EP_TYPE_INTERRUPT, HID_IN_EPSIZE, 1);
}

static void Endpoint_Read_And_Clear_Control_Stream_LE(uint8_t* buffer, size_t length)
{
	// TODO alternative
	//Endpoint_Read_Control_Stream_LE(&buffer, length);
	//return;

	// TODO comment
	Endpoint_ClearSETUP();

	// Store the data in the temporary buffer
	for (size_t i = 0; i < length; i++)
	{
		// Check if endpoint is empty - if so clear it and wait until ready for next packet
		if (!(Endpoint_BytesInEndpoint()))
		{
			Endpoint_ClearOUT();
			while (!(Endpoint_IsOUTReceived()));
		}

		// Get next data byte
		buffer[i] = Endpoint_Read_8();
	}

	// Acknowledge reading to the host
	Endpoint_ClearOUT();
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	// Ignore any requests that aren't directed to the HID interface
	// HostToDevice or DeviceToHost is unimportant as we use Set/GetReport
	if ((USB_ControlRequest.bmRequestType & (CONTROL_REQTYPE_TYPE | CONTROL_REQTYPE_RECIPIENT)) !=
	    (REQTYPE_CLASS | REQREC_INTERFACE))
	{
		return;
	}

	// Differentiate between Out and Feature report (in and reserved ignored)
	uint8_t reportType = (USB_ControlRequest.wValue >> 8);
	uint16_t length = USB_ControlRequest.wLength;

	/* Process HID specific control requests */
	switch (USB_ControlRequest.bRequest)
	{
		case HID_REQ_SetReport:
		{
			if(length == sizeof(ReadFlashPage.PageAddress)){
				//uart_putchars("PageAddress\r\n");
				Endpoint_Read_And_Clear_Control_Stream_LE(ReadFlashPage.raw, sizeof(ReadFlashPage.PageAddress));

				// TODO this check needs to be ported to > 0xFFFF
				if (ReadFlashPage.PageAddress < BOOT_START_ADDR){
					ReadFlashPage.PageAddress = 0; // TODO useful?
					// TODO else error, wrong page address -> stall to notice the app
				}

				// Check if the command is a program page command, or a start application command
				else if (ReadFlashPage.PageAddress == COMMAND_STARTAPPLICATION)
				{
					RunBootloader = false;
				}
			}
			else if(length == sizeof(ProgrammFlashPage)){
				//uart_putchars("ProgrammFlashPage\r\n");
				Endpoint_Read_And_Clear_Control_Stream_LE(ProgrammFlashPage.raw, sizeof(ProgrammFlashPage));

				// Read in the write destination address
				#if (FLASHEND > USHRT_MAX)
				uint32_t PageAddress = ((uint32_t)ProgrammFlashPage.PageAddress << 8);
				#else
				uint16_t PageAddress = ProgrammFlashPage.PageAddress;
				#endif

				//hexdump(&PageAddress, sizeof(PageAddress));

				// Check if the command is a program page command, or a start application command
				//TODO only use reboot with 2 byte command?
				#if (FLASHEND > USHRT_MAX)
				if ((uint16_t)(PageAddress >> 8) == COMMAND_STARTAPPLICATION)
				#else
				if (PageAddress == COMMAND_STARTAPPLICATION)
				#endif
				{
					RunBootloader = false;
				}
				// Do not overwrite the bootloader or write out of bounds
				else if (PageAddress < BOOT_START_ADDR)
				{
					//hexdump(ProgrammFlashPage.cbcMac, sizeof(ProgrammFlashPage.cbcMac));
					//hexdump(&ProgrammFlashPage, sizeof(ProgrammFlashPage));

				  // Save key and initialization vector inside context
					// Calculate CBC-MAC
				  aes256CbcMacInit(&ctx, BootloaderKey);
					aes256CbcMacUpdate(&ctx, ProgrammFlashPage.raw, sizeof(ProgrammFlashPage) - sizeof(ProgrammFlashPage.cbcMac));

					// Only write data if CBC-MAC matches
					if(!aes256CbcMacCompare(&ctx, ProgrammFlashPage.cbcMac)){
						// TODO else error/timeout
						//uart_putchars("CBCERR\r\n");
						return;
					}

					// Only write data if CBC-MAC matches
					// if(!aes256CbcMacInitUpdateCompare(&ctx, BootloaderKey,
					// 																	ProgrammFlashPage.raw,
					// 																	sizeof(ProgrammFlashPage) - sizeof(ProgrammFlashPage.cbcMac),
					// 																	ProgrammFlashPage.cbcMac)
					// {
					// 	// TODO else error/timeout
					// 	//uart_putchars("CBCERR\r\n");
					// 	return;
					// }

					//uart_putchars("Programming\r\n");
					BootloaderAPI_EraseFillWritePage(PageAddress, ProgrammFlashPage.PageData);
				}
				else{
					//uart_putchars("FlashPageErr\r\n");
					//Endpoint_StallTransaction(); //TODO move down as default and return on no error?
				}
			}
			else if(length == sizeof(changeBootloaderKey)){
				return; //TODO remove
				//uart_putchars("NewBKe\r\n");
				Endpoint_Read_And_Clear_Control_Stream_LE(changeBootloaderKey.raw, sizeof(changeBootloaderKey));

				// Save key and initialization vector inside context
				// Calculate CBC-MAC
				aes256CbcMacInit(&ctx, BootloaderKey);
				aes256CbcMacUpdate(&ctx, changeBootloaderKey.raw, sizeof(changeBootloaderKey));

				// Check if CBC-MAC matches
				uint8_t i = 0;
				for(i = 0; i < sizeof(ctx.cbcMac); i++){
					if(changeBootloaderKey.cbcMac[i] != ctx.cbcMac[i]){
						break;
					}
				}

				// Only write data if CBC-MAC is correct
				if(i != sizeof(ctx.cbcMac)){
					// TODO else error/timeout
					//uart_putchars("NBKERR\r\n");
					return;
				}

				//TODO decrypt key
			}
			else{
				//uart_putchars("Length error\r\n");
				//hexdump(&length, sizeof(length));
				return;
			}

			// Acknowledge SetReport request
			Endpoint_ClearStatusStage();
			break;
		}

		// TODO get report for checksum/authentification?
		// case HID_REQ_GetReport:
		// {
		// 	// Read in data via feature report
		// 	if(reportType != HID_REPORT_ITEM_Feature)
		// 		return;
		//
		// 	uint16_t PageAddress = chunk.PageAddress;
		// 	if (!(PageAddress < BOOT_START_ADDR)){
		// 		return;
		// 	}
		//
		// 	/* Read the next FLASH byte from the current FLASH page */
		// 	for (uint8_t PageWord = 0; PageWord < (SPM_PAGESIZE / 2); PageWord++){
		// 		#if (FLASHEND > USHRT_MAX)
		// 		chunk.pageData.pageBuff[PageWord] = pgm_read_word_far(PageWord);
		// 		#else
		// 		chunk.pageData.pageBuff[PageWord] = pgm_read_word(PageWord);
		// 		#endif
		// 	}
		//
		// 	// Save key and initialization vector inside context
		// 	aes256CbcMacInit(&ctx, key);
		//
		// 	// Calculate CBC-MAC
		// 	aes256CbcMac(&ctx, chunk.raw, sizeof(chunk.PageAddress) + sizeof(chunk.pageData.pageBuff));
		//
		// 	// Send the firmware flash to the PC
		// 	// TODO also send CBC-MAC?
		// 	for (size_t i = 0; i < sizeof(chunk); i++){
		//
		// 	}
		// 	break;
		// }
	}
}
