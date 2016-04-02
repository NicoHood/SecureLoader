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

#include "Endpoint.h"

//TODO 8 bit length?
void Endpoint_Write_Control_Stream_LE (const void* const Buffer,
                            uint16_t Length)
{
	uint8_t* DataStream     = ((uint8_t*)Buffer);
	bool     LastPacketFull = false;

  // TODO required? YES but why? -> descriptor
	if (Length > USB_ControlRequest.wLength)
	  Length = USB_ControlRequest.wLength;
	else if (!(Length)) //TODO remove and change loop below
	  Endpoint_ClearIN();

	while (Length || LastPacketFull)
	{
		if (Endpoint_IsINReady())
		{
			uint16_t BytesInEndpoint = Endpoint_BytesInEndpoint(); //TODO only use 8 bit?

			while (Length && (BytesInEndpoint < FIXED_CONTROL_ENDPOINT_SIZE))
			{
				Endpoint_Write_8(*DataStream);
				DataStream++;
				Length--;
				BytesInEndpoint++;
			}

			LastPacketFull = (BytesInEndpoint == FIXED_CONTROL_ENDPOINT_SIZE);
			Endpoint_ClearIN();
		}
	}
}
