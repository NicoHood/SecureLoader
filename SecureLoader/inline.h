
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
				Endpoint_ClearStatusStage();
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
				Endpoint_ClearStatusStage();
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
				Endpoint_ClearStatusStage();
			}
			// No valid data length found
			else{
				return;
			}

			// Acknowledge SetReport request
			Endpoint_ClearStatusStage();
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

// TODO the return statements are WRONG
//
// static void USB_Device_SetAddress(void)
// {
// 	uint8_t DeviceAddress = (USB_ControlRequest.wValue & 0x7F);
//
// 	USB_Device_SetDeviceAddress(DeviceAddress);
//
// 	Endpoint_ClearSETUP();
//
// 	Endpoint_ClearStatusStage();
//
// 	while (!(Endpoint_IsINReady()));
//
// 	USB_Device_EnableDeviceAddress(DeviceAddress);
// }
//
// static void USB_Device_SetConfiguration(void)
// {
// 	#if defined(FIXED_NUM_CONFIGURATIONS)
// 	if ((uint8_t)USB_ControlRequest.wValue > FIXED_NUM_CONFIGURATIONS)
// 	  return;
// 	#else
// 	USB_Descriptor_Device_t* DevDescriptorPtr;
//
// 	#if defined(ARCH_HAS_MULTI_ADDRESS_SPACE)
// 		#if defined(USE_FLASH_DESCRIPTORS)
// 			#define MemoryAddressSpace  MEMSPACE_FLASH
// 		#elif defined(USE_EEPROM_DESCRIPTORS)
// 			#define MemoryAddressSpace  MEMSPACE_EEPROM
// 		#elif defined(USE_RAM_DESCRIPTORS)
// 			#define MemoryAddressSpace  MEMSPACE_RAM
// 		#else
// 			uint8_t MemoryAddressSpace;
// 		#endif
// 	#endif
//
// 	if (CALLBACK_USB_GetDescriptor((DTYPE_Device << 8), 0, (void*)&DevDescriptorPtr
// 	#if defined(ARCH_HAS_MULTI_ADDRESS_SPACE) && \
// 	    !(defined(USE_FLASH_DESCRIPTORS) || defined(USE_EEPROM_DESCRIPTORS) || defined(USE_RAM_DESCRIPTORS))
// 	                               , &MemoryAddressSpace
// 	#endif
// 	                               ) == NO_DESCRIPTOR)
// 	{
// 		return;
// 	}
//
// 	#if defined(ARCH_HAS_MULTI_ADDRESS_SPACE)
// 	if (MemoryAddressSpace == MEMSPACE_FLASH)
// 	{
// 		if (((uint8_t)USB_ControlRequest.wValue > pgm_read_byte(&DevDescriptorPtr->NumberOfConfigurations)))
// 		  return;
// 	}
// 	else if (MemoryAddressSpace == MEMSPACE_EEPROM)
// 	{
// 		if (((uint8_t)USB_ControlRequest.wValue > eeprom_read_byte(&DevDescriptorPtr->NumberOfConfigurations)))
// 		  return;
// 	}
// 	else
// 	{
// 		if ((uint8_t)USB_ControlRequest.wValue > DevDescriptorPtr->NumberOfConfigurations)
// 		  return;
// 	}
// 	#else
// 	if ((uint8_t)USB_ControlRequest.wValue > DevDescriptorPtr->NumberOfConfigurations)
// 	  return;
// 	#endif
// 	#endif
//
// 	Endpoint_ClearSETUP();
//
// 	USB_Device_ConfigurationNumber = (uint8_t)USB_ControlRequest.wValue;
//
// 	Endpoint_ClearStatusStage();
//
// 	Endpoint_ConfigureEndpoint(HID_IN_EPADDR, EP_TYPE_INTERRUPT, HID_IN_EPSIZE, 1);
// }
//
// static void USB_Device_GetConfiguration(void)
// {
// 	Endpoint_ClearSETUP();
//
// 	Endpoint_Write_8(USB_Device_ConfigurationNumber);
// 	Endpoint_ClearIN();
//
// 	Endpoint_ClearStatusStage();
// }
//
// #if !defined(NO_INTERNAL_SERIAL) && (USE_INTERNAL_SERIAL != NO_DESCRIPTOR)
// static void USB_Device_GetInternalSerialDescriptor(void)
// {
// 	struct
// 	{
// 		USB_Descriptor_Header_t Header;
// 		uint16_t                UnicodeString[INTERNAL_SERIAL_LENGTH_BITS / 4];
// 	} SignatureDescriptor;
//
// 	SignatureDescriptor.Header.Type = DTYPE_String;
// 	SignatureDescriptor.Header.Size = USB_STRING_LEN(INTERNAL_SERIAL_LENGTH_BITS / 4);
//
// 	USB_Device_GetSerialString(SignatureDescriptor.UnicodeString);
//
// 	Endpoint_ClearSETUP();
//
// 	Endpoint_Write_Control_Stream_LE(&SignatureDescriptor, sizeof(SignatureDescriptor));
// 	Endpoint_ClearOUT();
// }
// #endif
//
// static void USB_Device_GetDescriptor(void)
// {
// 	const void* DescriptorPointer;
// 	uint16_t    DescriptorSize;
//
// 	#if defined(ARCH_HAS_MULTI_ADDRESS_SPACE) && \
// 	    !(defined(USE_FLASH_DESCRIPTORS) || defined(USE_EEPROM_DESCRIPTORS) || defined(USE_RAM_DESCRIPTORS))
// 	uint8_t DescriptorAddressSpace;
// 	#endif
//
// 	#if !defined(NO_INTERNAL_SERIAL) && (USE_INTERNAL_SERIAL != NO_DESCRIPTOR)
// 	if (USB_ControlRequest.wValue == ((DTYPE_String << 8) | USE_INTERNAL_SERIAL))
// 	{
// 		USB_Device_GetInternalSerialDescriptor();
// 		return;
// 	}
// 	#endif
//
// 	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(USB_ControlRequest.wValue, USB_ControlRequest.wIndex,
// 	                                                 &DescriptorPointer
// 	#if defined(ARCH_HAS_MULTI_ADDRESS_SPACE) && \
// 	    !(defined(USE_FLASH_DESCRIPTORS) || defined(USE_EEPROM_DESCRIPTORS) || defined(USE_RAM_DESCRIPTORS))
// 	                                                 , &DescriptorAddressSpace
// 	#endif
// 													 )) == NO_DESCRIPTOR)
// 	{
// 		return;
// 	}
//
// 	Endpoint_ClearSETUP();
//
// 	#if defined(USE_RAM_DESCRIPTORS) || !defined(ARCH_HAS_MULTI_ADDRESS_SPACE)
// 	Endpoint_Write_Control_Stream_LE(DescriptorPointer, DescriptorSize);
// 	#elif defined(USE_EEPROM_DESCRIPTORS)
// 	Endpoint_Write_Control_EStream_LE(DescriptorPointer, DescriptorSize);
// 	#elif defined(USE_FLASH_DESCRIPTORS)
// 	Endpoint_Write_Control_PStream_LE(DescriptorPointer, DescriptorSize);
// 	#else
// 	if (DescriptorAddressSpace == MEMSPACE_FLASH)
// 	  Endpoint_Write_Control_PStream_LE(DescriptorPointer, DescriptorSize);
// 	else if (DescriptorAddressSpace == MEMSPACE_EEPROM)
// 	  Endpoint_Write_Control_EStream_LE(DescriptorPointer, DescriptorSize);
// 	else
// 	  Endpoint_Write_Control_Stream_LE(DescriptorPointer, DescriptorSize);
// 	#endif
//
// 	Endpoint_ClearOUT();
// }
//
// static void USB_Device_GetStatus(void)
// {
// 	uint8_t CurrentStatus = 0;
//
// 	switch (USB_ControlRequest.bmRequestType)
// 	{
// 		case (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE):
// 		{
// 			break;
// 		}
// 		case (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_ENDPOINT):
// 		{
// 			#if !defined(CONTROL_ONLY_DEVICE)
// 			uint8_t EndpointIndex = ((uint8_t)USB_ControlRequest.wIndex & ENDPOINT_EPNUM_MASK);
//
// 			if (EndpointIndex >= ENDPOINT_TOTAL_ENDPOINTS)
// 				return;
//
// 			Endpoint_SelectEndpoint(EndpointIndex);
//
// 			CurrentStatus = Endpoint_IsStalled();
//
// 			Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
// 			#endif
//
// 			break;
// 		}
// 		default:
// 			return;
// 	}
//
// 	Endpoint_ClearSETUP();
//
// 	Endpoint_Write_16_LE(CurrentStatus);
// 	Endpoint_ClearIN();
//
// 	Endpoint_ClearStatusStage();
// }
//
// static void USB_Device_ClearSetFeature(void)
// {
// 	switch (USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT)
// 	{
// 		#if !defined(CONTROL_ONLY_DEVICE)
// 		case REQREC_ENDPOINT:
// 		{
// 			if ((uint8_t)USB_ControlRequest.wValue == FEATURE_SEL_EndpointHalt)
// 			{
// 				uint8_t EndpointIndex = ((uint8_t)USB_ControlRequest.wIndex & ENDPOINT_EPNUM_MASK);
//
// 				if (EndpointIndex == ENDPOINT_CONTROLEP || EndpointIndex >= ENDPOINT_TOTAL_ENDPOINTS)
// 				  return;
//
// 				Endpoint_SelectEndpoint(EndpointIndex);
//
// 				if (Endpoint_IsEnabled())
// 				{
// 					if (USB_ControlRequest.bRequest == REQ_SetFeature)
// 					{
// 						Endpoint_StallTransaction();
// 					}
// 					else
// 					{
// 						Endpoint_ClearStall();
// 						Endpoint_ResetEndpoint(EndpointIndex);
// 						Endpoint_ResetDataToggle();
// 					}
// 				}
// 			}
//
// 			break;
// 		}
// 		#endif
// 		default:
// 			return;
// 	}
//
// 	Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
//
// 	Endpoint_ClearSETUP();
//
// 	Endpoint_ClearStatusStage();
// }

// ------------------

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

  // TODO merge with upper function
  Endpoint_ClearSETUP();

  Endpoint_Write_16_LE(CurrentStatus);
  Endpoint_ClearIN();

  // TODO replace clear status stage
  Endpoint_ClearStatusStage();
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

	Endpoint_ClearStatusStage();
}


// 32u4 datasheet section 22.7
static void USB_Device_SetAddress(void)
{
	// Get address value
	uint8_t DeviceAddress = (USB_ControlRequest.wValue & 0x7F);

	// Record Address, keep ADDEN cleared
	USB_Device_SetDeviceAddress(DeviceAddress);

	// TODO move upwards?
	Endpoint_ClearSETUP();

	// Send IN ZLP
	Endpoint_ClearStatusStage();

	// Wait for IN endpoint to get ready
	while (!(Endpoint_IsINReady()));

	// Enable USB device address
	USB_Device_EnableDeviceAddress();
}


static void USB_Device_GetDescriptor(void)
{
	const void* DescriptorPointer;
	uint16_t    DescriptorSize;

	// TODO add internal serial otherwise
	// #if !defined(NO_INTERNAL_SERIAL) && (USE_INTERNAL_SERIAL != NO_DESCRIPTOR)
	// if (USB_ControlRequest.wValue == ((DTYPE_String << 8) | USE_INTERNAL_SERIAL))
	// {
	// 	USB_Device_GetInternalSerialDescriptor();
	// 	return;
	// }
	// #endif

	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(USB_ControlRequest.wValue,
																									 USB_ControlRequest.wIndex,
																									 &DescriptorPointer)) == NO_DESCRIPTOR)
	{
		return;
	}

	Endpoint_ClearSETUP();

	// TODO improve write control stream/inline
	#if defined(USE_RAM_DESCRIPTORS)
	Endpoint_Write_Control_Stream_LE(DescriptorPointer, DescriptorSize);
	#elif defined(USE_EEPROM_DESCRIPTORS)
	#error not implemented
	Endpoint_Write_Control_EStream_LE(DescriptorPointer, DescriptorSize);
	#elif defined(USE_FLASH_DESCRIPTORS)
	#error not implemented
	Endpoint_Write_Control_PStream_LE(DescriptorPointer, DescriptorSize);
	#else
	// TODO improve this check
	#error no descriptor type specified
	#endif

	Endpoint_ClearOUT();
}

static void USB_Device_GetConfiguration(void)
{
	// TODO teensy waits for in ready?
	Endpoint_ClearSETUP();

	Endpoint_Write_8(USB_Device_ConfigurationNumber);
	Endpoint_ClearIN();

	// TODO teensy doesnt use this
	Endpoint_ClearStatusStage();
	// TODO will result in:
	// while (!(Endpoint_IsINReady()));
}

static void USB_Device_SetConfiguration(void)
{
	USB_Device_ConfigurationNumber = (uint8_t)USB_ControlRequest.wValue;

	// TODO is this check required?
	if (USB_Device_ConfigurationNumber > FIXED_NUM_CONFIGURATIONS)
		return;

	#if !defined(FIXED_NUM_CONFIGURATIONS)
	// TODO improve
	#error Please define FIXED_NUM_CONFIGURATIONS
	#endif

	Endpoint_ClearSETUP();

	Endpoint_ClearStatusStage();

	/* Setup HID Report Endpoint */
	//EVENT_USB_Device_ConfigurationChanged(); // TODO inlined
	// TODO inline configure function itself?
	Endpoint_ConfigureEndpoint(HID_IN_EPADDR, EP_TYPE_INTERRUPT, HID_IN_EPSIZE, 1);
}

void USB_Device_ProcessControlRequest(void)
{
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
					USB_Device_GetStatus(); // used, fully optimized
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
				  USB_Device_SetAddress(); //used, better than teensy, Endpoint_ClearSETUP might be moved
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
				  USB_Device_GetConfiguration(); //used, different TODO
				}

				break;
			case REQ_SetConfiguration:
				if (bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE))
				{
				  USB_Device_SetConfiguration(); //used
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
