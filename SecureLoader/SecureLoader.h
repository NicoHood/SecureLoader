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
 *  Header file for BootloaderHID.c.
 */

#ifndef _BOOTLOADERHID_H_
#define _BOOTLOADERHID_H_

    /* Includes: */
        #include <avr/io.h>
        #include <avr/wdt.h>
        #include <avr/boot.h>
        #include <avr/power.h>
        #include <avr/interrupt.h>
        #include <stdbool.h>
        #include <avr/eeprom.h>

        #include "AES/aes256.h"
        #include "SERIAL/serial.h"
        #include "BootloaderAPI.h"
        #include "Protocol.h"

        #include <USB/USB.h>

    /* Preprocessor Checks: */
        #if !defined(__OPTIMIZE_SIZE__)
            #error This bootloader requires that it be optimized for size, not speed, to fit into the target device. Change optimization settings and try again.
        #endif

    /* Macros: */
        /** Magic bootloader key to unlock forced application start mode. */
        #define MAGIC_BOOT_KEY	0x77

    /* Function Prototypes: */
        static void SetupHardware(void);

        void Application_Jump_Check(void) ATTR_INIT_SECTION(3);

        void EVENT_USB_Device_ConfigurationChanged(void);
        void EVENT_USB_Device_ControlRequest2(void);
        void USB_Device_ProcessControlRequestInline(void);

#endif
