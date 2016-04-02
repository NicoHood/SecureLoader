

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	// Ignore any requests that aren't directed to the HID interface
	// HostToDevice or DeviceToHost is unimportant as we use Set/GetReport
	if ((USB_ControlRequest.bmRequestType & (CONTROL_REQTYPE_TYPE | CONTROL_REQTYPE_RECIPIENT)) !=
	    (REQTYPE_CLASS | REQREC_INTERFACE))	{
		return;
	}

	// Get input data length
	// TODO also important for GET_Report?
	uint16_t length = USB_ControlRequest.wLength;

	/* Process HID specific control requests */
	switch (USB_ControlRequest.bRequest)
	{
		// Do not differentiate between Out or Feature report (in and reserved are ignored too)
		case HID_REQ_SetReport:
		{
			// Do not read more data than we have available as buffer
			if(length > sizeof(USB_Buffer)){
				return;
			}

			// Acknowledge setup data
			Endpoint_ClearSETUP();

			// Store the data in the temporary buffer
			for (size_t i = 0; i < length; i++)
			{
				// Check if endpoint is empty - if so clear it and wait until ready for next packet
				if (!(Endpoint_BytesInEndpoint()))
				{
					Endpoint_ClearOUT();
					while (!(Endpoint_IsOUTReceived()));
				}

				// Get next data byte
				USB_Buffer[i] = Endpoint_Read_8();
			}

			// Acknowledge reading to the host
			Endpoint_ClearOUT();

			// Process SetFlashPage command
			if(length == sizeof_member(SetFlashPage_t, PageAddress))
			{
				// Interpret data SetFlashPage_t
				SetFlashPage_t* SetFlashPage = (SetFlashPage_t*)USB_Buffer;
				uint16_t PageAddress = SetFlashPage->PageAddress;

				// Check if the command is a program page command, or a start application command.
				// Do not validate PageAddress, we do this in the GetReport request.
				if (PageAddress == COMMAND_STARTAPPLICATION) {
					RunBootloader = false;
				}

				// Acknowledge SetReport request
				Endpoint_ClearStatusStageHostToDevice();
				return;
			}
			// Process ProgrammFlashPage command
			else if(length == sizeof(ProgrammFlashPage_t))
			{
				// Interpret data as ProgrammFlashPage_t
				ProgrammFlashPage_t* ProgrammFlashPage = (ProgrammFlashPage_t*)USB_Buffer;

				// Read in the write destination address
				#if (FLASHEND > USHRT_MAX)
				uint32_t PageAddress = ((uint32_t)ProgrammFlashPage->PageAddress << 8);
				#else
				uint16_t PageAddress = ProgrammFlashPage->PageAddress;
				#endif

				// Do not overwrite the bootloader or write out of bounds
			 	if (PageAddress >= BOOT_START_ADDR)
				{
					Endpoint_StallTransaction();
					return;
				}

				// memcpy(ctx.cbcMac, ProgrammFlashPage->cbcMac, sizeof(ctx.cbcMac));
				//
				// // Initialize key schedule inside CTX
				// aes256_init(BootloaderKey, &(ctx.aesCtx));
				//
				// // Only write data if CBC-MAC matches
				// if(aes256CbcMacReverseCompare(&ctx, ProgrammFlashPage->raw,
				//  	                 						sizeof(ProgrammFlashPage_t) - sizeof_member(ProgrammFlashPage_t, cbcMac)))
				// {
				// 	// TODO timeout, prevent brute force
				// 	Endpoint_StallTransaction();
				// 	return;
				// }

			  // Save key and initialization vector inside context
				// Calculate CBC-MAC
			  aes256CbcMacInit(&ctx, BootloaderKey);
				aes256CbcMacUpdate(&ctx, ProgrammFlashPage->raw,
					                 sizeof(ProgrammFlashPage_t) - sizeof_member(ProgrammFlashPage_t, cbcMac));

				// Only write data if CBC-MAC matches
				if(aes256CbcMacCompare(&ctx, ProgrammFlashPage->cbcMac)){
					// TODO timeout, prevent brute force
					Endpoint_StallTransaction();
					return;
				}

				// // Only write data if CBC-MAC matches
				// if(aes256CbcMacInitUpdateCompare(&ctx, BootloaderKey,
				// 																	ProgrammFlashPage->raw,
				// 																	sizeof(ProgrammFlashPage_t) - sizeof_member(ProgrammFlashPage_t, cbcMac),
				// 																	ProgrammFlashPage->cbcMac))
				// {
				// 	// TODO timeout, prevent brute force
				//  Endpoint_StallTransaction();
				//  return;
				// }

				//uart_putchars("Programming\r\n");
				BootloaderAPI_EraseFillWritePage(PageAddress, ProgrammFlashPage->PageData);

				// Acknowledge SetReport request
				Endpoint_ClearStatusStageHostToDevice();
				break;
			}
			// Process changeBootloaderKey command
			else if(length == sizeof(changeBootloaderKey_t))
			{
				return;
				// Interpret data as ProgrammFlashPage_t
				changeBootloaderKey_t* changeBootloaderKey = (changeBootloaderKey_t*)USB_Buffer;

				// Initialize key schedule inside CTX
				aes256_init(BootloaderKey, &(ctx.aesCtx));

				for(uint8_t i = 0; i < (sizeof(changeBootloaderKey_t) / AES256_CBC_LENGTH); i++){
					// Encrypt next block
			    //aes256_dec(changeBootloaderKey->raw + (i * AES256_CBC_LENGTH), &(ctx.aesCtx));
				}

				hexdump(changeBootloaderKey->raw, sizeof(changeBootloaderKey_t));
				return;




				// Save key and initialization vector inside context
				// Calculate CBC-MAC
				aes256CbcMacInit(&ctx, BootloaderKey);
				aes256CbcMacUpdate(&ctx, changeBootloaderKey->raw, sizeof(changeBootloaderKey_t) - sizeof_member(ProgrammFlashPage_t, cbcMac));

				// Only continue if CBC-MAC matches
				if(aes256CbcMacCompare(&ctx, changeBootloaderKey->cbcMac)){
					// TODO timeout, prevent brute force
					Endpoint_StallTransaction();
					return;
				}

				// Only continue if CBC-MAC matches
				// if(aes256CbcMacInitUpdateCompare(&ctx, BootloaderKey,
				// 																	changeBootloaderKey->raw,
				// 																	sizeof(changeBootloaderKey_t) - sizeof_member(ProgrammFlashPage_t, cbcMac),
				// 																	changeBootloaderKey->cbcMac))
				// {
				// 	// TODO timeout, prevent brute force
				// 	Endpoint_StallTransaction();
				// 	return;
				// }

				//TODO decrypt key

				// Acknowledge SetReport request
				Endpoint_ClearStatusStageHostToDevice();
			}
			// No valid data length found
			else{
				return;
			}

			// Acknowledge SetReport request
			Endpoint_ClearStatusStageHostToDevice();
			break;
		}

		// TODO get report for checksum/authentification?
		// case HID_REQ_GetReport:
		// {
		// 	// Read in data via feature report
		// 	if(reportType != HID_REPORT_ITEM_Feature)
		// 		return;
		//
		// 	uint16_t PageAddress = chunk.PageAddress;
		// 	if (!(PageAddress < BOOT_START_ADDR)){
		// 		return;
		// 	}
		// TODO this check needs to be ported to > 0xFFFF
		// Check if the command is a program page command, or a start application command
		// #if (FLASHEND > USHRT_MAX)
		// if ((uint16_t)(PageAddress >> 8) == COMMAND_STARTAPPLICATION)
		// #else
		// if (PageAddress == COMMAND_STARTAPPLICATION)
		// #endif
		// {
		// 	RunBootloader = false;
		// }
		//
		// 	/* Read the next FLASH byte from the current FLASH page */
		// 	for (uint8_t PageWord = 0; PageWord < (SPM_PAGESIZE / 2); PageWord++){
		// 		#if (FLASHEND > USHRT_MAX)
		// 		chunk.pageData.pageBuff[PageWord] = pgm_read_word_far(PageWord);
		// 		#else
		// 		chunk.pageData.pageBuff[PageWord] = pgm_read_word(PageWord);
		// 		#endif
		// 	}
		//
		// 	// Save key and initialization vector inside context
		// 	aes256CbcMacInit(&ctx, key);
		//
		// 	// Calculate CBC-MAC
		// 	aes256CbcMac(&ctx, chunk.raw, sizeof(chunk.PageAddress) + sizeof(chunk.pageData.pageBuff));
		//
		// 	// Send the firmware flash to the PC
		// 	// TODO also send CBC-MAC?
		// 	for (size_t i = 0; i < sizeof(chunk); i++){
		//
		// 	}
		// 	break;
		// }
	}
}



// USB2.0 section 9.4.5 page 254
static void USB_Device_GetStatus(void)
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
static void USB_Device_ClearSetFeature(void)
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
				//>= ENDPOINT_TOTAL_ENDPOINTS
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
static void USB_Device_SetAddress(void)
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


static void USB_Device_GetDescriptor(void)
{
	const void* DescriptorPointer;
	uint16_t    DescriptorSize;

	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(USB_ControlRequest.wValue,
																									 USB_ControlRequest.wIndex,
																									 &DescriptorPointer)) == NO_DESCRIPTOR)
	{
		return;
	}

	Endpoint_ClearSETUP();

	// TODO improve write control stream/inline
	Endpoint_Write_Control_Stream_LE(DescriptorPointer, DescriptorSize);

	Endpoint_ClearStatusStageDeviceToHost();
}

static void USB_Device_GetConfiguration(void)
{
	Endpoint_ClearSETUP();

	Endpoint_Write_8(USB_Device_ConfigurationNumber);
	Endpoint_ClearIN();

	Endpoint_ClearStatusStageDeviceToHost();
}

static void USB_Device_SetConfiguration(void)
{
	USB_Device_ConfigurationNumber = (uint8_t)USB_ControlRequest.wValue;

	if (USB_Device_ConfigurationNumber > FIXED_NUM_CONFIGURATIONS)
		return;

	Endpoint_ClearSETUP();

	Endpoint_ClearStatusStageHostToDevice();

	/* Setup HID Report Endpoint */
	Endpoint_ConfigureEndpoint(HID_IN_EPADDR, EP_TYPE_INTERRUPT, HID_IN_EPSIZE, 1);
}

void USB_Device_ProcessControlRequest(void)
{
	// TODO inline again?
	uint8_t* RequestHeader = (uint8_t*)&USB_ControlRequest;

	for (uint8_t RequestHeaderByte = 0; RequestHeaderByte < sizeof(USB_Request_Header_t); RequestHeaderByte++)
	  *(RequestHeader++) = Endpoint_Read_8();

	EVENT_USB_Device_ControlRequest();

	if (Endpoint_IsSETUPReceived())
	{
		uint8_t bmRequestType = USB_ControlRequest.bmRequestType;

		switch (USB_ControlRequest.bRequest)
		{
			case REQ_GetStatus:
				if ((bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) ||
					(bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_ENDPOINT)))
				{
					return; // TODO doesnt seem to be essential
					USB_Device_GetStatus(); // optional, fully optimized
				}

				break;
			case REQ_ClearFeature:
			case REQ_SetFeature:
				if ((bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE)) ||
					(bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_ENDPOINT)))
				{
					USB_Device_ClearSetFeature(); //used, fully optimized
				}

				break;
			case REQ_SetAddress:
				if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
				{
				  USB_Device_SetAddress(); // essential, fully optimized, better than teensy
				}
				break;
			case REQ_GetDescriptor:
				if ((bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) ||
					(bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE)))
				{
					USB_Device_GetDescriptor(); //used, different, sending can be improved a lot TODO
				}

				break;
			case REQ_GetConfiguration:
				if (bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE))
				{
					return; // TODO doesnt seem to be essential
				  USB_Device_GetConfiguration(); // optional, fully optomized
				}

				break;
			case REQ_SetConfiguration:
				if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
				{
				  USB_Device_SetConfiguration(); // essential, fully optomized
				}

				break;

			default:
				break;
		}
	}

	if (Endpoint_IsSETUPReceived())
	{
		Endpoint_ClearSETUP();
		Endpoint_StallTransaction();
	}
}
