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

							// Stop if the endpoint is not used
							if (EndpointIndex == ENDPOINT_CONTROLEP || EndpointIndex > FIXED_NUM_ENDPOINTS)
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
				const uint8_t DescriptorNumber = (USB_ControlRequest.wValue & 0xFF);

				// If/Else chain compiles slightly smaller than a switch case
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
				else if (DescriptorType == HID_DTYPE_Report)
				{
					DescriptorAddress = &HIDReport;
					DescriptorSize    = sizeof(HIDReport);
				}
				else if (DescriptorType == DTYPE_String)
				{
					if (DescriptorNumber == STRING_ID_Language)
					{
						DescriptorAddress = &LanguageString;
						DescriptorSize    = LanguageString.Header.Size;
					}
					else if (DescriptorNumber == STRING_ID_Manufacturer)
					{
						DescriptorAddress = &ManufacturerString;
						DescriptorSize    = ManufacturerString.Header.Size;
					}
					else if (DescriptorNumber == STRING_ID_Product)
					{
						DescriptorAddress = &ProductString;
						DescriptorSize    = ProductString.Header.Size;
					}
					else if (DescriptorNumber == STRING_ID_Serial)
					{
						DescriptorAddress = &SerialString;
						DescriptorSize    = SerialString.Header.Size;
					}
				}

				// Abort if no suitable descriptor was found
				if (DescriptorSize == NO_DESCRIPTOR)
				{
					return;
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
#endif
			}


	/* Public Interface - May be used in end-application: */
		/** Event for control requests. This event fires when a the USB host issues a control request
			*  to the mandatory device control endpoint (of address 0). This may either be a standard
			*  request that the library may have a handler code for internally, or a class specific request
			*  issued to the device which must be handled appropriately. If a request is not processed in the
			*  user application via this event, it will be passed to the library for processing internally
			*  if a suitable handler exists.
			*
			*  This event is time-critical; each packet within the request transaction must be acknowledged or
			*  sent within 50ms or the host will abort the transfer.
			*
			*  The library internally handles all standard control requests with the exceptions of SYNC FRAME,
			*  SET DESCRIPTOR and SET INTERFACE. These and all other non-standard control requests will be left
			*  for the user to process via this event if desired. If not handled in the user application or by
			*  the library internally, unknown requests are automatically STALLed.
			*
			*  \note Requests should be handled in the same manner as described in the USB 2.0 Specification,
			*        or appropriate class specification. In all instances, the library has already read the
			*        request SETUP parameters into the \ref USB_ControlRequest structure which should then be used
			*        by the application to determine how to handle the issued request.
			*/
			static inline void EVENT_USB_Device_ControlRequest(void);

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
								// TODO doesnt seem to be essential
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

				// Select control endpoint again
				Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

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
