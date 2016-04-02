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

#ifndef __USBCONTROLLER_H__
#define __USBCONTROLLER_H__

	/* Includes: */
		#include "Common/Common.h"
		#include "USBMode.h" 			// USB_SERIES
		#include "USBInterrupt.h" // USB_INT_Enable()
		#include "USBGlobals.h" 	// USB_Device_ConfigurationNumber

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks and Defines: */
		#if !defined(F_USB)
			#error F_USB is not defined. You must define F_USB to the frequency of the unprescaled USB controller clock in your project makefile.
		#endif

		#if (F_USB == 8000000)
			#if (defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__) || \
			     defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__) || \
			     defined(__AVR_ATmega32U2__))
				#define USB_PLL_PSC                0
			#elif (defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__))
				#define USB_PLL_PSC                0
			#elif (defined(__AVR_AT90USB646__)  || defined(__AVR_AT90USB1286__))
				#define USB_PLL_PSC                ((1 << PLLP1) | (1 << PLLP0))
			#elif (defined(__AVR_AT90USB647__)  || defined(__AVR_AT90USB1287__))
				#define USB_PLL_PSC                ((1 << PLLP1) | (1 << PLLP0))
			#endif
		#elif (F_USB == 16000000)
			#if (defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__) || \
			     defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__) || \
			     defined(__AVR_ATmega32U2__))
				#define USB_PLL_PSC                (1 << PLLP0)
			#elif (defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__))
				#define USB_PLL_PSC                (1 << PINDIV)
			#elif (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__))
				#define USB_PLL_PSC                ((1 << PLLP2) | (1 << PLLP1))
			#elif (defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__))
				#define USB_PLL_PSC                ((1 << PLLP2) | (1 << PLLP0))
			#endif
		#endif

		#if !defined(USB_PLL_PSC)
			#error No PLL prescale value available for chosen F_USB value and AVR model.
		#endif

	/* Public Interface - May be used in end-application: */
		/* Inline Functions: */
			/** Detaches the device from the USB bus. This has the effect of removing the device from any
			 *  attached host, ceasing USB communications. If no host is present, this prevents any host from
			 *  enumerating the device once attached until \ref USB_Attach() is called.
			 */
			static inline void USB_Detach(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Detach(void)
			{
				UDCON  |=  (1 << DETACH);
			}

			/** Attaches the device to the USB bus. This announces the device's presence to any attached
			 *  USB host, starting the enumeration process. If no host is present, attaching the device
			 *  will allow for enumeration once a host is connected to the device.
			 *
			 *  This is inexplicably also required for proper operation while in host mode, to enable the
			 *  attachment of a device to the host. This is despite the bit being located in the device-mode
			 *  register and despite the datasheet making no mention of its requirement in host mode.
			 */
			static inline void USB_Attach(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Attach(void)
			{
				UDCON  &= ~(1 << DETACH);
			}

			static inline void USB_PLL_On(void) ATTR_ALWAYS_INLINE;
			static inline void USB_PLL_On(void)
			{
				PLLCSR = USB_PLL_PSC;
				PLLCSR = (USB_PLL_PSC | (1 << PLLE));
			}

			static inline bool USB_PLL_IsReady(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline bool USB_PLL_IsReady(void)
			{
				return ((PLLCSR & (1 << PLOCK)) ? true : false);
			}

			static inline void USB_REG_On(void) ATTR_ALWAYS_INLINE;
			static inline void USB_REG_On(void)
			{
			#if defined(USB_SERIES_4_AVR) || defined(USB_SERIES_6_AVR)
				UHWCON |=  (1 << UVREGE);
			#else
				REGCR  &= ~(1 << REGDIS);
			#endif
			}

			#if defined(USB_SERIES_4_AVR) || defined(USB_SERIES_6_AVR)
			static inline void USB_OTGPAD_On(void) ATTR_ALWAYS_INLINE;
			static inline void USB_OTGPAD_On(void)
			{
				USBCON |=  (1 << OTGPADE);
			}
			#endif

			static inline void USB_CLK_Unfreeze(void) ATTR_ALWAYS_INLINE;
			static inline void USB_CLK_Unfreeze(void)
			{
				USBCON &= ~(1 << FRZCLK);
			}

			static inline void USB_Controller_Enable(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Controller_Enable(void)
			{
				USBCON |=  (1 << USBE);
			}

			/** Main function to initialize and start the USB interface. Once active, the USB interface will
			 *  allow for device connection to a host.
			 *
			 *  As the USB library relies on interrupts for the device and host mode enumeration processes,
			 *  the user must enable global interrupts before or shortly after this function is called. In
			 *  device mode, interrupts must be enabled within 500ms of this function being called to ensure
			 *  that the host does not time out whilst enumerating the device.
			 *
			 *  Calling this function when the USB interface is already initialized is not intended to be used.
			 */
			static inline void USB_Init(void);
			// See section 21.13, page 265 USB Software Operating modes of the 32u4 datasheet
			static inline void USB_Init(void)
			{
				// Initialize reserved register
				USB_Device_ConfigurationNumber = 0;

				// Power-On USB pads regulator
				USB_REG_On();

				// Enable USB interface
				USB_Controller_Enable();

				// Enable PLL
				USB_PLL_On();

				// Check PLL lock
				while (!(USB_PLL_IsReady()));

				// Unfreeze USB clock
				USB_CLK_Unfreeze();

				// Configure USB interface (USB speed, Endpoints configuration...)
				#if (defined(USB_SERIES_4_AVR) || defined(USB_SERIES_6_AVR))
					// Not required, initial value is FullSpeed.
					//USB_Device_SetFullSpeed();
				#endif

				// TODO Teensy and Arduino do not use this -> only in interrupt (works)
				// https://github.com/PaulStoffregen/cores/issues/121
				// https://github.com/abcminiuser/lufa/commit/d66a925786e4e57713a2c902cdffe7269db43226
				//Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL,
				//							 FIXED_CONTROL_ENDPOINT_SIZE, 1);

				// Enable reset interrupt
				USB_INT_Enable(USB_INT_EORSTI);

				// Wait for USB VBUS information connection
				#if (defined(USB_SERIES_4_AVR) || defined(USB_SERIES_6_AVR))
				USB_OTGPAD_On();
				#endif

				// Attach USB device
				USB_Attach();
			}

	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif

/** @} */
