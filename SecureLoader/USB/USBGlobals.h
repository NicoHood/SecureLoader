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

#ifndef __USBGLOBALS_H__
#define __USBGLOBALS_H__

	/* Includes: */
		#include "Common/Common.h"
    #include "StdRequestType.h" // USB_Request_Header_t

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_USB_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB/USB.h instead.
		#endif

  /* Public Interface - May be used in end-application: */
    /* Global Variables: */
      /** Indicates the currently set configuration number of the device. USB devices may have several
       *  different configurations which the host can select between; this indicates the currently selected
       *  value, or 0 if no configuration has been selected.
       *
       *  \attention This variable should be treated as read-only in the user application, and never manually
       *             changed in value.
       *
       *  \ingroup Group_Device
       */
      extern uint8_t USB_Device_ConfigurationNumber;

      /** Structure containing the last received Control request when in Device mode (for use in user-applications
       *  inside of the \ref EVENT_USB_Device_ControlRequest() event, or for filling up with a control request to
       *  issue when in Host mode before calling \ref USB_Host_SendControlRequest().
       *
       *  \note The contents of this structure is automatically endian-corrected for the current CPU architecture.
       *
       *  \ingroup Group_USBManagement
       */
       extern USB_Request_Header_t USB_ControlRequest;

   /* Disable C linkage for C++ Compilers: */
     #if defined(__cplusplus)
       }
     #endif

#endif
