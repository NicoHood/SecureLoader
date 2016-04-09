/*
                         LUFA Library
         Copyright (C) Dean Camera, 2016.

    dean [at] fourwalledcubicle [dot] com
                     www.lufa-lib.org
*/

/*
    Copyright 2016    Dean Camera (dean [at] fourwalledcubicle [dot] com)

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
    and fitness.    In no event shall the author be liable for any
    special, indirect or consequential damages or any damages
    whatsoever resulting from loss of use, data or profits, whether
    in an action of contract, negligence or other tortious action,
    arising out of or in connection with the use or performance of
    this software.
*/

/** \file
 *
 *    Main source file for the HID class bootloader. This file contains the complete bootloader logic.
 */

#include "BootloaderHID.h"

/** Flag to indicate if the bootloader should be running, or should exit and allow the application code to run
 *    via a soft reset. When cleared, the bootloader will abort, the USB interface will shut down and the application
 *    started via a forced watchdog reset.
 */
static bool RunBootloader = true;

/** Magic lock for forced application start. If the HWBE fuse is programmed and BOOTRST is unprogrammed, the bootloader
 *  will start if the /HWB line of the AVR is held low and the system is reset. However, if the /HWB line is still held
 *  low when the application attempts to start via a watchdog reset, the bootloader will re-start. If set to the value
 *  \ref MAGIC_BOOT_KEY the special init function \ref Application_Jump_Check() will force the application to start.
 */
static volatile uint8_t* MagicBootKeyPtr = (volatile uint8_t *)RAMEND;

static uint8_t CheckButton ATTR_NO_INIT;


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

static ProgrammFlashPage_t ProgrammFlashPage;

// Set a flash page address, that can be requested by the host afterwards
typedef union
{
    uint8_t raw[0];
    struct
    {
        uint16_t PageAddress;
    };
} SetFlashPage_t;

static SetFlashPage_t SetFlashPage = { .PageAddress = 0xFFFF };

// Data to read a flash page that was requested by the host
typedef union
{
    uint8_t raw[0];
    struct
    {
        uint16_t PageAddress;
        uint16_t PageData[SPM_PAGESIZE/2];
    };
} ReadFlashPage_t;

static ReadFlashPage_t ReadFlashPage;

// Data to change the Bootloader Key
typedef union
{
    uint8_t raw[0];
    struct
    {
        //uint8_t IV[AES256_CBC_LENGTH];
        uint8_t BootloaderKey[32];
        uint8_t Mac[AES256_CBC_LENGTH];
    };
} changeBootloaderKey_t;

static changeBootloaderKey_t changeBootloaderKey;

#ifdef USE_EEPROM_KEY
// TODO set proper eeprom address space via makefile
static uint8_t EEMEM BootloaderKeyEEPROM[32] =
{
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b,    0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b,    0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

// Bootloader Key (local ram copy)
static uint8_t BootloaderKeyRam[32];

#else

// Data to change the Bootloader Key
typedef union
{
    uint8_t raw[0];
    uint8_t bytes[SPM_PAGESIZE];
    uint16_t words[SPM_PAGESIZE/2];
    struct
    {
        uint8_t padding[SPM_PAGESIZE - 2 - 32];
        uint8_t HardwareButton;
        uint8_t HardwareButtonPadding;
        uint8_t BootloaderKey[32];
    };
} secureBootloaderSection_t;

static secureBootloaderSection_t SBS;

#endif

static void readSBS(void)
{
    // Load PROGMEM data into temporary SBS RAM structure
    BootloaderAPI_ReadPage(FLASHEND - SPM_PAGESIZE + 1, SBS.raw);
}

static void writeSBS(void)
{
    // Write local RAM copy of SBS back to PROGMEM
    BootloaderAPI_EraseFillWritePage(FLASHEND - SPM_PAGESIZE + 1, SBS.words);
}

static void initSBS(void) __attribute__ ((used, naked, section (".init5")));
static void initSBS(void)
{
    // Load PROGMEM data of SBS into RAM.
    // This has to be done after .init4 section!
    readSBS();
}

// AES256 CBC-MAC context variable
static aes256CbcMacCtx_t ctx;

//#define USE_EEPROM_KEY

#define PORTID_BUTTON                PORTE6
#define PORT_BUTTON                    PORTE
#define DDR_BUTTON                     DDRE
#define PIN_BUTTON                     PINE

static inline bool ButtonPressed(void)
{
    // Check if low
    return !(PIN_BUTTON & (1 << PORTID_BUTTON));
}

static inline bool JumpToBootloader(void)
{
    // TODO check if HW button setting is enabled
    // TODO or use a timeout?

    // Pressing button starts the bootloader
    DDR_BUTTON &= ~(1 << PORTID_BUTTON);
    PORT_BUTTON |= (1 << PORTID_BUTTON);

    return ButtonPressed();
}

/** Special startup routine to check if the bootloader was started via a watchdog reset, and if the magic application
 *    start key has been loaded into \ref MagicBootKey. If the bootloader started via the watchdog and the key is valid,
 *    this will force the user application to start via a software jump.
 */
void Application_Jump_Check(void)
{
    // Turn off the watchdog, save reset source
    uint8_t mcusr_state = MCUSR;
    MCUSR = 0;
    wdt_disable();

    // On a watchdog reset check if the application requested the bootloader
    uint8_t MagicBootKey = *MagicBootKeyPtr;
    *MagicBootKeyPtr = 0x00;
    if ((mcusr_state & (1 << WDRF)) && (MagicBootKey == MAGIC_BOOT_KEY))
    {
        return;
    }

    // Start bootloader if hardware button was pressed at startup
    CheckButton = 0;
    if(JumpToBootloader())
    {
        CheckButton = 1;
        return;
    }

    // Don't run the user application if the reset vector is blank
    bool ApplicationValid = (pgm_read_word_near(0) != 0xFFFF);
    if (ApplicationValid)
    {
        // Clear RAM
        for (uint8_t* p = (uint8_t*)RAMSTART; p <= (uint8_t*)RAMEND; p++)
        {
            *p = 0x00;
        }

        // Start application
        ((void (*)(void))0x0000)();
    }
}


/** Main program entry point. This routine configures the hardware required by the bootloader, then continuously
 *    runs the bootloader processing routine until instructed to soft-exit.
 */
int main(void)
{
    // TODO startup delay
    // TODO disconnect on error

    uart_init();

    uart_putchars("Start---------------\r\n");
    hexdump(SBS.raw, sizeof(SBS));


    /* Setup hardware required for the bootloader */
    SetupHardware();

    /* Enable global interrupts so that the USB stack can function */
    GlobalInterruptEnable();

    // Process USB data
    do{
#if !defined(INTERRUPT_CONTROL_ENDPOINT)
        USB_Device_ProcessControlRequest();
#endif

        // Use a timeout if hardware button was used to enter bootloader mode
        if (CheckButton)
        {
            // Wait check approx 2,5 ~ 3 seconds button to exit bootloader
            if(!ButtonPressed())
            {
                CheckButton++;
                if(!CheckButton)
                {
                    RunBootloader = false;
                }
                _delay_ms(10);
            }
            else
            {
                // Reset timer if button is pressed again
                CheckButton = true;
            }
        }
    } while (RunBootloader);

    // Disconnect from the host.
    // USB interface will be reset later along with the AVR.
    USB_Detach();

    // Enable the watchdog and force a timeout to reset the AVR
    wdt_enable(WDTO_250MS);

    for (;;);
}

#if defined(INTERRUPT_CONTROL_ENDPOINT)
ISR(USB_COM_vect, ISR_BLOCK)
{
    USB_Device_ProcessControlRequest();
}
#endif

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

static void initAES(void)
{
    #ifdef USE_EEPROM_KEY
    // (Re)load the EEPROM Bootloader Key inside RAM TODO move? reimplement with 8 bit
    eeprom_read_block((void*)BootloaderKeyRam, (const void*)BootloaderKeyEEPROM, sizeof(BootloaderKeyRam));

    // Initialize key schedule inside CTX
    aes256_init(BootloaderKeyRam, &(ctx.aesCtx));

    #else
    // (Re)load the PROGMEM Bootloader Key inside RAM
    //readSBS();

    // Initialize key schedule inside CTX
    aes256_init(SBS.BootloaderKey, &(ctx.aesCtx));
    #endif
}


void ReadUSBData(uint8_t* buf, uint16_t length)
{
    // Acknowledge setup data
    Endpoint_ClearSETUP();

    Endpoint_Read_Control_Stream_LE(buf, length);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *    the device from the USB host before passing along unhandled control requests to the library for processing
 *    internally.
 */
static inline void EVENT_USB_Device_ControlRequest(void)
{
    // Get input data length
    uint16_t length = USB_ControlRequest.wLength;

    /* Process HID specific control requests */
    switch (USB_ControlRequest.bRequest)
    {
        // Do not differentiate between Out or Feature report (in and reserved are ignored too)
        case HID_REQ_SetReport:
        {
            // Process SetFlashPage command
            if (length == sizeof(SetFlashPage))
            {
                // Read in data
                ReadUSBData(SetFlashPage.raw, sizeof(SetFlashPage));

                // Check if the command is a program page command, or a start application command.
                // Do not validate PageAddress, we do this in the GetReport request.
                if (SetFlashPage.PageAddress == COMMAND_STARTAPPLICATION)
                {
                    RunBootloader = false;
                }
            }
            // Process ProgrammFlashPage command
            else if (length == sizeof(ProgrammFlashPage))
            {
                ReadUSBData(ProgrammFlashPage.raw, sizeof(ProgrammFlashPage));

                // Do not overwrite the bootloader or write out of bounds
                address_size_t PageAddress = getPageAddress(ProgrammFlashPage.PageAddress);
                if ((PageAddress >= BOOT_START_ADDR) || (PageAddress & (SPM_PAGESIZE - 1)))
                {
                    Endpoint_StallTransaction();
                    return;
                }

                initAES();

                // Loop will update cbcMac for each block
                uint16_t dataLen = sizeof(ProgrammFlashPage) - sizeof(ProgrammFlashPage.cbcMac);
                for (uint16_t i=0; i<dataLen; i+=AES256_CBC_LENGTH)
                {
                    // Decrypt next block
                    aes256_dec(ProgrammFlashPage.cbcMac, &(ctx.aesCtx));

                    // XOR cbcMac with data
                    aesXorVectors(ProgrammFlashPage.cbcMac, ProgrammFlashPage.raw + dataLen - i - AES256_CBC_LENGTH, AES256_CBC_LENGTH);
                }

                // Check if CBC-MAC matches
                for (uint8_t i = 0; i < AES256_CBC_LENGTH; i++)
                {
                    // TODO for security reasons the padding should also be checked if zero?
                    // TODO Move the cbc mac at the beginning to check them together
                    if (ProgrammFlashPage.cbcMac[i] != 0x00)
                    {
                        Endpoint_StallTransaction();
                        return;
                    }
                }

                // Programm flash page
                BootloaderAPI_EraseFillWritePage(PageAddress, ProgrammFlashPage.PageDataWords);
            }
            // Process changeBootloaderKeyRam command
            else if (length == sizeof(changeBootloaderKey))
            {
                ReadUSBData(changeBootloaderKey.raw, sizeof(changeBootloaderKey));
                initAES();

                // Decrypt all blocks
                // TODO use CBC
                for (uint8_t i = 0; i < (sizeof(changeBootloaderKey) / AES256_CBC_LENGTH); i++)
                {
                    aes256_dec(changeBootloaderKey.raw + (i * AES256_CBC_LENGTH), &(ctx.aesCtx));
                }

                // TODO prev 3670

                // // Loop will update cbcMac for each block
                // // TODO check/set IV -> dont use usb buffer, use special static structs
                // uint16_t dataLen = sizeof(changeBootloaderKey_t);
                // for (uint16_t i = dataLen - AES256_CBC_LENGTH; i > 0; i -= AES256_CBC_LENGTH)
                // {
                //     // Decrypt next block
                //     aes256_dec(changeBootloaderKey->raw + i, &(ctx.aesCtx));
                //
                //     // XOR data with previous block
                //     aesXorVectors(ProgrammFlashPage->raw + i, ProgrammFlashPage->raw + i - AES256_CBC_LENGTH, AES256_CBC_LENGTH);
                // }
                //
                // // Check if CBC-MAC matches
                // for (uint8_t i = 0; i < AES256_CBC_LENGTH; i++){
                //     // TODO for security reasons the padding should also be checked if zero?
                //     // TODO Move the cbc mac at the beginning to check them together
                //     if (ProgrammFlashPage->cbcMac[i] != 0x00){
                //         Endpoint_StallTransaction();
                //         return;
                //     }
                // }

                // Check if MAC matches (0-15)
                for (uint8_t i = 0; i < AES256_CBC_LENGTH; i++)
                {
                    if (changeBootloaderKey.Mac[i] != i)
                    {
                        Endpoint_StallTransaction();
                        return;
                    }
                }

                //uart_putchars("key\r\n");

                #ifdef USE_EEPROM_KEY
                // Write new BootloaderKey to EEPROM
                // TODO use write, as a BK change is only available for authorized people
                BootloaderAPI_UpdateEEPROM(changeBootloaderKey.raw, &BootloaderKeyEEPROM, sizeof(BootloaderKeyEEPROM));

                #else
                // Write new BootloaderKey to PROGMEM (SBS)
                memcpy(SBS.BootloaderKey, changeBootloaderKey.BootloaderKey, sizeof(SBS.BootloaderKey));
                writeSBS();
                #endif

                //readSBS();
                //hexdump(SBS.raw, sizeof(SBS));
            }
            // No valid data length found
            else
            {
                Endpoint_StallTransaction();
                return;
            }

            // Acknowledge SetReport request
            Endpoint_ClearStatusStageHostToDevice();
            break;
        }

        case HID_REQ_GetReport:
        {
            // Only response to feature request that request a flash page.
            // Set the page address first via SetReport.
            if ((uint8_t)(USB_ControlRequest.wValue >> 8) != HID_REPORT_ITEM_Feature
                 || length >= sizeof(ReadFlashPage))
            {
                // Acknowledge setup data
                Endpoint_ClearSETUP();

                // Read the source address
                address_size_t PageAddress = getPageAddress(SetFlashPage.PageAddress);

                // Do not overwrite the bootloader or write out of bounds
                if ((PageAddress >= BOOT_START_ADDR) || (PageAddress & (SPM_PAGESIZE - 1)))
                {
                    Endpoint_StallTransaction();
                    return;
                }

                // Read flash page into temporary buffer
                BootloaderAPI_ReadPage(SetFlashPage.PageAddress, ReadFlashPage.raw);

                // Write the page data to the PC
                Endpoint_Write_Control_Stream_LE(ReadFlashPage.raw, sizeof(ReadFlashPage_t));

                // Acknowledge GetReport request
                Endpoint_ClearStatusStageDeviceToHost();
            }
            break;
        }
        default:
        {
            Endpoint_StallTransaction();
            return;
        }
    }

    // No error, valid HID command was used. Stay in bootloader mode.
    CheckButton = 0;
}
