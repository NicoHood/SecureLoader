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

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

	/* Includes: */
		#include "StdDescriptors.h" // USB_CONFIG_POWER_MA
		#include "HIDClass.h" 			// HID_RI_USAGE_PAGE
		#include "Endpoint.h" 			// ENDPOINT_DIR_IN

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_USB_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB/USB.h instead.
		#endif

	/* Type Defines: */
		/** Type define for the device configuration descriptor structure. This must be defined in the
		 *  application code, as the configuration descriptor contains several sub-descriptors which
		 *  vary between devices, and which describe the device's usage to the host.
		 */
		typedef struct
		{
			USB_Descriptor_Configuration_Header_t Config;

			// Generic HID Interface
			USB_Descriptor_Interface_t            HID_Interface;
			USB_HID_Descriptor_HID_t              HID_VendorHID;
			USB_Descriptor_Endpoint_t             HID_ReportINEndpoint;
		} USB_Descriptor_Configuration_t;

		/** Enum for the device interface descriptor IDs within the device. Each interface descriptor
		 *  should have a unique ID index associated with it, which can be used to refer to the
		 *  interface from other descriptors.
		 */
		enum InterfaceDescriptors_t
 		{
 			INTERFACE_ID_GenericHID = 0, /**< GenericHID interface descriptor ID */
 		};

	/* Macros: */
		/** Endpoint address of the HID data IN endpoint. */
		#define HID_IN_EPADDR                (ENDPOINT_DIR_IN | 1)

		/** Size in bytes of the HID reporting IN endpoint. */
		#define HID_IN_EPSIZE                64

		/** HID class report descriptor. This is a special descriptor constructed with values from the
		 *  USBIF HID class specification to describe the reports and capabilities of the HID device. This
		 *  descriptor is parsed by the host and its contents used to determine what data (and in what encoding)
		 *  the device will send, and what it may be sent back from the host. Refer to the HID specification for
		 *  more details on HID report descriptors.
		 */
		// TODO linux does not require this descriptor at all??
		#include "AES/aes256_ctr.h"
		static const USB_Descriptor_HIDReport_Datatype_t HIDReport[] =
		{
			HID_RI_USAGE_PAGE(16, 0xFFDC), /* Vendor Page 0xDC */
			HID_RI_USAGE(8, 0xFB), /* Vendor Usage 0xFB */
			HID_RI_COLLECTION(8, 0x01), /* Vendor Usage 1 */
				HID_RI_USAGE(8, 0x02), /* Vendor Usage 2 */
				HID_RI_LOGICAL_MINIMUM(8, 0x00),
				HID_RI_LOGICAL_MAXIMUM(8, 0xFF),
				HID_RI_REPORT_SIZE(8, 0x08),
				HID_RI_REPORT_COUNT(16, (sizeof(uint16_t) + SPM_PAGESIZE + AES256_CBC_LENGTH)), //TODO
				HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE | HID_IOF_NON_VOLATILE),
			HID_RI_END_COLLECTION(0),
		};

		/** Device descriptor structure. This descriptor, located in SRAM memory, describes the overall
		 *  device characteristics, including the supported USB version, control endpoint size and the
		 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
		 *  process begins.
		 */
		static const USB_Descriptor_Device_t DeviceDescriptor =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

			.USBSpecification       = VERSION_BCD(1,1,0), //TODO USB 2.0?
			.Class                  = USB_CSCP_NoDeviceClass,
			.SubClass               = USB_CSCP_NoDeviceSubclass,
			.Protocol               = USB_CSCP_NoDeviceProtocol,

			.Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

			.VendorID               = VENDORID,
			.ProductID              = PRODUCTID,
			.ReleaseNumber          = VERSION_BCD(0,0,1),

			.ManufacturerStrIndex   = NO_DESCRIPTOR, //TODO add
			.ProductStrIndex        = NO_DESCRIPTOR,
			.SerialNumStrIndex      = NO_DESCRIPTOR,

			.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
		};

		/** Configuration descriptor structure. This descriptor, located in SRAM memory, describes the usage
		 *  of the device in one of its supported configurations, including information about any device interfaces
		 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
		 *  a configuration so that the host may correctly communicate with the USB device.
		 */
		static const USB_Descriptor_Configuration_t ConfigurationDescriptor =
		{
			.Config =
				{
					.Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

					.TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
					.TotalInterfaces        = 1,

					.ConfigurationNumber    = 1,
					.ConfigurationStrIndex  = NO_DESCRIPTOR,

					.ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,

					.MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
				},

			.HID_Interface =
				{
					.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

					.InterfaceNumber        = INTERFACE_ID_GenericHID,
					.AlternateSetting       = 0x00,

					.TotalEndpoints         = 1,

					.Class                  = HID_CSCP_HIDClass,
					.SubClass               = HID_CSCP_NonBootSubclass,
					.Protocol               = HID_CSCP_NonBootProtocol,

					.InterfaceStrIndex      = NO_DESCRIPTOR
				},

			.HID_VendorHID =
				{
					.Header                 = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},

					.HIDSpec                = VERSION_BCD(1,1,1),
					.CountryCode            = 0x00,
					.TotalReportDescriptors = 1,
					.HIDReportType          = HID_DTYPE_Report,
					.HIDReportLength        = sizeof(HIDReport)
				},

			.HID_ReportINEndpoint =
				{
					.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

					.EndpointAddress        = HID_IN_EPADDR,
					.Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
					.EndpointSize           = HID_IN_EPSIZE,
					.PollingIntervalMS      = 0x05
				},
		};

	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif
