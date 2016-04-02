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
 *  \brief USB mode and feature support definitions.
 *  \copydetails Group_USBMode
 *
 *  \note This file should not be included directly. It is automatically included as needed by the USB driver
 *        dispatch header located in LUFA/Drivers/USB/USB.h.
 */

/** \ingroup Group_USB
 *  \defgroup Group_USBMode USB Mode Tokens
 *  \brief USB mode and feature support definitions.
 *
 *  This file defines macros indicating the type of USB controller the library is being compiled for, and its
 *  capabilities. These macros may then be referenced in the user application to selectively enable or disable
 *  code sections depending on if they are defined or not.
 *
 *  After the inclusion of the master USB driver header, one or more of the following tokens may be defined, to
 *  allow the user code to conditionally enable or disable code based on the USB controller family and allowable
 *  USB modes. These tokens may be tested against to eliminate code relating to a USB mode which is not enabled for
 *  the given compilation.
 *
 *  @{
 */

#ifndef __USBMODE_H__
#define __USBMODE_H__

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_USB_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB/USB.h instead.
		#endif

		/* Macros: */
			#if (defined(__AVR_AT90USB162__) || defined(__AVR_AT90USB82__)  || \
			     defined(__AVR_ATmega32U2__) || defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega8U2__))
				#define USB_SERIES_2_AVR
			#elif (defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__))
				#define USB_SERIES_4_AVR
			#elif (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__))
				#define USB_SERIES_6_AVR
			#endif

	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif

/** @} */
