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

#define  __INCLUDE_FROM_USB_DRIVER

#include "USBGlobals.h"   // Headers
#include "Endpoint.h"     // Endpoint_ConfigureEndpoint
#include "USBInterrupt.h" // USB_INT_ClearAllInterrupts
#include "DeviceStandardReq.h" 	// USB_Device_ProcessControlRequest

USB_Request_Header_t USB_ControlRequest;
uint8_t USB_Device_ConfigurationNumber;

ISR(USB_GEN_vect, ISR_BLOCK)
{
	// Get interrupt source and clear it
	uint8_t UDINT_mirror = UDINT;
	USB_INT_ClearAllInterrupts();

	// Reset Interrupt
	if (UDINT_mirror & (1 << EORSTI))
	{
		USB_Device_ConfigurationNumber = 0;

		Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL,
		                           FIXED_CONTROL_ENDPOINT_SIZE, 1);

		#if defined(INTERRUPT_CONTROL_ENDPOINT)
		USB_INT_Enable(USB_INT_RXSTPI);
		#endif
	}
}

#if defined(INTERRUPT_CONTROL_ENDPOINT)
ISR(USB_COM_vect, ISR_BLOCK)
{
	USB_Device_ProcessControlRequest();

	#if !defined(CONTROL_ONLY_DEVICE)
		#error Previous selected endpoints will not be restored.
		#error Add this feature or only use this if you know what you are doing.
		#error Remove this error to accept.
		#warning Interrupt control Endpoint uses ~150-200 more bytes of flash
	#endif
}
#endif
