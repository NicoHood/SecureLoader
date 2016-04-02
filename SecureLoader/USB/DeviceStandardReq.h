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

#ifndef __DEVICESTDREQ_H__
#define __DEVICESTDREQ_H__

	/* Includes: */
		#include "Common/Common.h"
		#include "Device.h" 			// USB_Device_EnableDeviceAddress
		#include "USBGlobals.h" 	// USB_ControlRequest
		#include "Endpoint.h" 		// Endpoint_ConfigureEndpoint
		#include "Descriptors.h" 	// DeviceDescriptor

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_USB_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB/USB.h instead.
		#endif

		// TODO prototype for:
		//void EVENT_USB_Device_ControlRequest(void);


	/* Private Interface - For use in library only: */
			// USB2.0 section 9.4.5 page 254
			static inline void USB_Device_GetStatus(void);
			static inline void USB_Device_GetStatus(void)
			{
			  uint8_t CurrentStatus = 0;

			  switch (USB_ControlRequest.bmRequestType)
			  {
			    case (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE):
			    {
			      // Just send a status with zeros.
			      // We do not have remote wakeup or self powered enabled.
			      break;
			    }
			    case (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE):
			    {
			      // If an interface is required, the response should be zeros too.
			      break;
			    }
			    case (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_ENDPOINT):
			    {
			      #if !defined(CONTROL_ONLY_DEVICE)
			      Endpoint_SelectEndpoint((uint8_t)USB_ControlRequest.wIndex & ENDPOINT_EPNUM_MASK);

			      CurrentStatus = Endpoint_IsStalled();

			      Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
			      #endif

			      break;
			    }
			    default:
			    {
			      // Anything else is undefined and we will just flag an error.
			      return;
			    }
			  }

			  Endpoint_ClearSETUP();

			  Endpoint_Write_16_LE(CurrentStatus);
			  Endpoint_ClearIN();

			  Endpoint_ClearStatusStageDeviceToHost();
			}


			// USB2.0 section 9.4.9 page 258
			static inline void USB_Device_ClearSetFeature(void);
			static inline void USB_Device_ClearSetFeature(void)
			{
				switch (USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT)
				{
					#if !defined(CONTROL_ONLY_DEVICE)
					case REQREC_ENDPOINT:
						if ((uint8_t)USB_ControlRequest.wValue == FEATURE_SEL_EndpointHalt)
						{
							uint8_t EndpointIndex = ((uint8_t)USB_ControlRequest.wIndex & ENDPOINT_EPNUM_MASK);

							// We ignore errors for non existing endpoints
							//TODO
							//  || EndpointIndex > ENDPOINT_TOTAL_ENDPOINTS
							if (EndpointIndex == ENDPOINT_CONTROLEP)
								return;

							Endpoint_SelectEndpoint(EndpointIndex);

							if (Endpoint_IsEnabled())
							{
								if (USB_ControlRequest.bRequest == REQ_SetFeature)
								{
									Endpoint_StallTransaction();
								}
								else
								{
									Endpoint_ClearStall();
									Endpoint_ResetEndpoint(EndpointIndex);
									Endpoint_ResetDataToggle();
								}
							}
						}

						break;
					#endif
					default:
						return;
				}

				Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

				Endpoint_ClearSETUP();

				Endpoint_ClearStatusStageHostToDevice();
			}


			// 32u4 datasheet section 22.7
			static inline void USB_Device_SetAddress(void);
			static inline void USB_Device_SetAddress(void)
			{
				// Get address value
				uint8_t DeviceAddress = (USB_ControlRequest.wValue & 0x7F);

				// Record Address, keep ADDEN cleared
				USB_Device_SetDeviceAddress(DeviceAddress);

				Endpoint_ClearSETUP();

				// Send IN ZLP
				Endpoint_ClearStatusStageHostToDevice();

				// Wait for IN endpoint to get ready
				while (!(Endpoint_IsINReady()));

				// Enable USB device address
				USB_Device_EnableDeviceAddress();
			}


			static inline void USB_Device_GetDescriptor(void);
			static inline void USB_Device_GetDescriptor(void)
			{
				const void* DescriptorAddress = NULL;
				uint16_t    DescriptorSize = NO_DESCRIPTOR;

			  const uint8_t DescriptorType   = (USB_ControlRequest.wValue >> 8);

				/* If/Else chain compiles slightly smaller than a switch case */
				if (DescriptorType == DTYPE_Device)
				{
					DescriptorAddress = &DeviceDescriptor;
					DescriptorSize    = sizeof(USB_Descriptor_Device_t);
				}
				else if (DescriptorType == DTYPE_Configuration)
				{
					DescriptorAddress = &ConfigurationDescriptor;
					DescriptorSize    = sizeof(USB_Descriptor_Configuration_t);
				}
				else if (DescriptorType == HID_DTYPE_HID)
				{
					DescriptorAddress = &ConfigurationDescriptor.HID_VendorHID;
					DescriptorSize    = sizeof(USB_HID_Descriptor_HID_t);
				}
				// TODO vendor and product strings? TODO internal string
			  // TODO why defaulting, what about returning?
				else
				{
					DescriptorAddress = &HIDReport;
					DescriptorSize    = sizeof(HIDReport);
				}

				Endpoint_ClearSETUP();

				Endpoint_Write_Control_Stream_LE(DescriptorAddress, DescriptorSize);

				Endpoint_ClearStatusStageDeviceToHost();
			}


			static inline void USB_Device_GetConfiguration(void);
			static inline void USB_Device_GetConfiguration(void)
			{
				Endpoint_ClearSETUP();

				Endpoint_Write_8(USB_Device_ConfigurationNumber);
				Endpoint_ClearIN();

				Endpoint_ClearStatusStageDeviceToHost();
			}


			static inline void USB_Device_SetConfiguration(void);
			static inline void USB_Device_SetConfiguration(void)
			{
				USB_Device_ConfigurationNumber = (uint8_t)USB_ControlRequest.wValue;

				if (USB_Device_ConfigurationNumber > FIXED_NUM_CONFIGURATIONS)
					return;

				Endpoint_ClearSETUP();

				Endpoint_ClearStatusStageHostToDevice();

#if !defined(CONTROL_ONLY_DEVICE)
				/* Setup HID Report Endpoint */
				Endpoint_SelectEndpoint(HID_IN_EPADDR & ENDPOINT_EPNUM_MASK);
				Endpoint_ConfigureEndpoint(HID_IN_EPADDR, EP_TYPE_INTERRUPT, HID_IN_EPSIZE, 1);
				Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
#endif
			}


		/* Public Interface - May be used in end-application: */
			static inline void USB_Device_ProcessControlRequest(void);
			static inline void USB_Device_ProcessControlRequest(void)
			{
				if (!Endpoint_IsSETUPReceived()){
					return;
				}

				uint8_t* RequestHeader = (uint8_t*)&USB_ControlRequest;

				for (uint8_t RequestHeaderByte = 0; RequestHeaderByte < sizeof(USB_Request_Header_t); RequestHeaderByte++)
				  *(RequestHeader++) = Endpoint_Read_8();

			  uint8_t bmRequestType = USB_ControlRequest.bmRequestType;

			  // Ignore any requests that aren't directed to the HID interface
				// HostToDevice or DeviceToHost is unimportant as we use Set/GetReport
				if ((bmRequestType & (CONTROL_REQTYPE_TYPE | CONTROL_REQTYPE_RECIPIENT)) ==
				    (REQTYPE_CLASS | REQREC_INTERFACE))	{
					EVENT_USB_Device_ControlRequest();
				}
			  else
				{
					switch (USB_ControlRequest.bRequest)
					{
						case REQ_GetStatus:
							if ((bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) ||
								(bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_ENDPOINT)))
							{
								USB_Device_GetStatus();
							}
							break;

						case REQ_ClearFeature:
						case REQ_SetFeature:
							if ((bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE)) ||
								(bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_ENDPOINT)))
							{
								USB_Device_ClearSetFeature();
							}
							break;

						case REQ_SetAddress:
							if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
							{
							  USB_Device_SetAddress();
							}
							break;

						case REQ_GetDescriptor:
							if ((bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) ||
								(bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE)))
							{
								USB_Device_GetDescriptor();
							}
							break;

						case REQ_GetConfiguration:
							if (bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE))
							{
								return; // TODO doesnt seem to be essential
							  USB_Device_GetConfiguration();
							}
							break;

						case REQ_SetConfiguration:
							if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
							{
							  USB_Device_SetConfiguration();
							}
							break;

						default:
							break;
					}
				}

				// TODO simpler call stall
				if (Endpoint_IsSETUPReceived())
				{
					Endpoint_ClearSETUP();
					Endpoint_StallTransaction();
				}
			}


	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif
