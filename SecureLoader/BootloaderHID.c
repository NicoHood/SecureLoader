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
		uint8_t Mac[AES256_CBC_LENGTH];
	};
} changeBootloaderKey_t;

// Temporary USB_Buffer holds data to send/receive data.
// It gets overwritten with every request.
// Since more features use the same buffer you can
// set an PageAddress via several methods (SetFlashPage or ProgrammFlashPage).
static uint8_t USB_Buffer[MAX(MAX(sizeof(ReadFlashPage_t), sizeof(ProgrammFlashPage_t)),
													sizeof(changeBootloaderKey_t))];

// TODO set proper eeprom address space via makefile
static uint8_t EEMEM BootloaderKeyEEPROM[32] = {
	0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
	0x2b,	0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
	0x1f, 0x35, 0x2c, 0x07, 0x3b,	0x61, 0x08, 0xd7,
	0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

// Bootloader Key (local ram copy)
static uint8_t BootloaderKeyRam[32];


// AES256 CBC-MAC context variable
static aes256CbcMacCtx_t ctx;

#define PORTID_BUTTON				 PORTE6
#define PORT_BUTTON          PORTE
#define DDR_BUTTON           DDRE
#define PIN_BUTTON           PINE

static inline bool JumpToBootloader(void)
{
	// Pressing button starts the bootloader
	DDR_BUTTON &= ~(1 << PORTID_BUTTON);
	PORT_BUTTON |= (1 << PORTID_BUTTON);

	// Check if low
	return !(PIN_BUTTON & (1 << PORTID_BUTTON));
}

/** Special startup routine to check if the bootloader was started via a watchdog reset, and if the magic application
 *  start key has been loaded into \ref MagicBootKey. If the bootloader started via the watchdog and the key is valid,
 *  this will force the user application to start via a software jump.
 */
void Application_Jump_Check(void)
{
	// Turn off the watchdog
  MCUSR = 0;
  wdt_disable();

	// Don't run the user application if the reset vector is blank (no app loaded)
	bool ApplicationValid = (pgm_read_word_near(0) != 0xFFFF);

	// Start apllication if available and no button was pressed at startup
	if(ApplicationValid && !JumpToBootloader()){
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

	// Process USB data
	do{
#if !defined(INTERRUPT_CONTROL_ENDPOINT)
		USB_Device_ProcessControlRequest();
#endif
	} while (RunBootloader);

	// Disconnect from the host - USB interface will be reset later along with the AVR
	USB_Detach();

	// Enable the watchdog and force a timeout to reset the AVR
	wdt_enable(WDTO_250MS);

	for (;;);
}

/** Configures all hardware required for the bootloader. */
static void SetupHardware(void)
{
#if F_CPU != F_USB
	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#endif

	/* Relocate the interrupt vector table to the bootloader section */
	MCUCR = (1 << IVCE);
	MCUCR = (1 << IVSEL);

	/* Initialize USB subsystem */
	USB_Init();
}


/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
static inline void EVENT_USB_Device_ControlRequest(void)
{
	// Get input data length
	// TODO also important for GET_Report?
	uint16_t length = USB_ControlRequest.wLength;

	/* Process HID specific control requests */
	switch (USB_ControlRequest.bRequest)
	{
		// Do not differentiate between Out or Feature report (in and reserved are ignored too)
		case HID_REQ_SetReport:
		{
			// Do not read more data than we have available as buffer
			if(length > sizeof(USB_Buffer)){
				return;
			}

			// Acknowledge setup data
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
				USB_Buffer[i] = Endpoint_Read_8();
			}

			// Acknowledge reading to the host
			Endpoint_ClearOUT();

			// (Re)load the eeprom bootloader key inside eeprom TODO move?
			eeprom_read_block((void*)BootloaderKeyRam, (const void*)BootloaderKeyEEPROM, sizeof(BootloaderKeyRam));

			// Initialize key schedule inside CTX
			// TODO inline with eeprom read above
			aes256_init(BootloaderKeyRam, &(ctx.aesCtx));

			// Process SetFlashPage command
			if(0){
				//TODO remove
			}
			// else if(length == sizeof_member(SetFlashPage_t, PageAddress))
			// {
			// 	// Interpret data SetFlashPage_t
			// 	SetFlashPage_t* SetFlashPage = (SetFlashPage_t*)USB_Buffer;
			// 	uint16_t PageAddress = SetFlashPage->PageAddress;
			//
			// 	// Check if the command is a program page command, or a start application command.
			// 	// Do not validate PageAddress, we do this in the GetReport request.
			// 	if (PageAddress == COMMAND_STARTAPPLICATION) {
			// 		RunBootloader = false;
			// 	}
			// }
			// Process ProgrammFlashPage command
			else if(length == sizeof(ProgrammFlashPage_t))
			{
				// Interpret data as ProgrammFlashPage_t
				ProgrammFlashPage_t* ProgrammFlashPage = (ProgrammFlashPage_t*)USB_Buffer;
				uint16_t InputPageAddress = ProgrammFlashPage->PageAddress;

				// TODO move to feature request?
				if (InputPageAddress == COMMAND_STARTAPPLICATION) {
					RunBootloader = false;
				}

				// Read in the write destination address
				#if (FLASHEND > USHRT_MAX)
				uint32_t PageAddress = ((uint32_t)InputPageAddress << 8);
				#else
				uint16_t PageAddress = InputPageAddress;
				#endif

				// Do not overwrite the bootloader or write out of bounds
				// TODO move too write function?
			 	if (PageAddress >= BOOT_START_ADDR)
				{
					Endpoint_StallTransaction();
					return;
				}

				// Loop will update cbcMac for each block
				uint16_t dataLen = sizeof_member(ProgrammFlashPage_t, PageData) + sizeof_member(ProgrammFlashPage_t, cbcMac);
			  for (uint16_t i=0; i<dataLen; i+=AES256_CBC_LENGTH)
			  {
			    // Decrypt next block
			    aes256_dec(ProgrammFlashPage->cbcMac, &(ctx.aesCtx));

			    // XOR cbcMac with data
			    aesXorVectors(ProgrammFlashPage->cbcMac, ProgrammFlashPage->raw + dataLen - i - AES256_CBC_LENGTH, AES256_CBC_LENGTH);
			  }

				// Check if CBC-MAC matches
				for(uint8_t i = 0; i < AES256_CBC_LENGTH; i++){
					// TODO for security reasons the padding should also be checked if zero
					// Move the cbc mac at the beginning to check them together
					if(ProgrammFlashPage->cbcMac[i] != 0x00){
						Endpoint_StallTransaction();
						return;
					}
				}


				// memcpy(ctx.cbcMac, ProgrammFlashPage->cbcMac, sizeof(ctx.cbcMac));
				//
				// // Initialize key schedule inside CTX
				// //aes256_init(BootloaderKeyRam, &(ctx.aesCtx));
				//
				// // Only write data if CBC-MAC matches
				// if(aes256CbcMacReverseCompare(&ctx, ProgrammFlashPage->raw,
				//  	                 						sizeof(ProgrammFlashPage_t) - sizeof_member(ProgrammFlashPage_t, cbcMac)))
				// {
				// 	// TODO timeout, prevent brute force
				// 	Endpoint_StallTransaction();
				// 	return;
				// }

			  // // Save key and initialization vector inside context
				// // Calculate CBC-MAC
			  // aes256CbcMacInit(&ctx, BootloaderKeyRam);
				// aes256CbcMacUpdate(&ctx, ProgrammFlashPage->raw,
				// 	                 sizeof(ProgrammFlashPage_t) - sizeof_member(ProgrammFlashPage_t, cbcMac));
				//
				// // Only write data if CBC-MAC matches
				// if(aes256CbcMacCompare(&ctx, ProgrammFlashPage->cbcMac)){
				// 	// TODO timeout, prevent brute force
				// 	Endpoint_StallTransaction();
				// 	return;
				// }

				// // Only write data if CBC-MAC matches
				// if(aes256CbcMacInitUpdateCompare(&ctx, BootloaderKeyRam,
				// 																	ProgrammFlashPage->raw,
				// 																	sizeof(ProgrammFlashPage_t) - sizeof_member(ProgrammFlashPage_t, cbcMac),
				// 																	ProgrammFlashPage->cbcMac))
				// {
				// 	// TODO timeout, prevent brute force
				//  Endpoint_StallTransaction();
				//  return;
				// }

				//uart_putchars("Programming\r\n");
				BootloaderAPI_EraseFillWritePage(PageAddress, ProgrammFlashPage->PageData);
			}
			// Process changeBootloaderKeyRam command
			else if(length == sizeof(changeBootloaderKey_t))
			{
				// Interpret data as ProgrammFlashPage_t
				changeBootloaderKey_t* changeBootloaderKey = (changeBootloaderKey_t*)USB_Buffer;

				// Decrypt all blocks
				for(uint8_t i = 0; i < (sizeof(changeBootloaderKey_t) / AES256_CBC_LENGTH); i++){
			    aes256_dec(changeBootloaderKey->raw + (i * AES256_CBC_LENGTH), &(ctx.aesCtx));
				}

				// Check if MAC matches (0-15)
				for(uint8_t i = 0; i < AES256_CBC_LENGTH; i++){
					if(changeBootloaderKey->Mac[i] != i){
						// TODO this stall is ignored. it will not change the key, but will not cause an error
						// TODO timeout
						Endpoint_StallTransaction();
						return;
					}
				}

				// TODO reimplement for 8 bit
				// Write new BootloaderKey to EEPROM
				eeprom_update_block (changeBootloaderKey->raw, &BootloaderKeyEEPROM, sizeof(BootloaderKeyEEPROM));


				// // Save key and initialization vector inside context
				// // Calculate CBC-MAC
				// aes256CbcMacInit(&ctx, BootloaderKeyRam);
				// aes256CbcMacUpdate(&ctx, changeBootloaderKey->raw, sizeof(changeBootloaderKey_t) - sizeof_member(ProgrammFlashPage_t, cbcMac));
				//
				// // Only continue if CBC-MAC matches
				// if(aes256CbcMacCompare(&ctx, changeBootloaderKey->Mac)){
				// 	// TODO timeout, prevent brute force
				// 	Endpoint_StallTransaction();
				// 	return;
				// }

				// Only continue if CBC-MAC matches
				// if(aes256CbcMacInitUpdateCompare(&ctx, BootloaderKeyRam,
				// 																	changeBootloaderKey->raw,
				// 																	sizeof(changeBootloaderKey_t) - sizeof_member(ProgrammFlashPage_t, cbcMac),
				// 																	changeBootloaderKey->cbcMac))
				// {
				// 	// TODO timeout, prevent brute force
				// 	Endpoint_StallTransaction();
				// 	return;
				// }

				//TODO decrypt key

				// Acknowledge SetReport request
				//Endpoint_ClearStatusStageHostToDevice();
			}
			// No valid data length found
			else{
				Endpoint_StallTransaction();
				return;
			}

			// Acknowledge SetReport request
			Endpoint_ClearStatusStageHostToDevice();
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
		// TODO this check needs to be ported to > 0xFFFF
		// Check if the command is a program page command, or a start application command
		// #if (FLASHEND > USHRT_MAX)
		// if ((uint16_t)(PageAddress >> 8) == COMMAND_STARTAPPLICATION)
		// #else
		// if (PageAddress == COMMAND_STARTAPPLICATION)
		// #endif
		// {
		// 	RunBootloader = false;
		// }
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
