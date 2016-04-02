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

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

	/* Includes: */
		#include "Common/Common.h"
		#include "Device.h" // DEVICE_STATE
		#include "DeviceStandardReq.h" // USB_ControlRequest

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_USB_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB/USB.h instead.
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Endpoint number mask, for masking against endpoint addresses to retrieve the endpoint's
			 *  numerical address in the device.
			 */
			#define ENDPOINT_EPNUM_MASK                     0x0F

			/** Endpoint address for the default control endpoint, which always resides in address 0. This is
			 *  defined for convenience to give more readable code when used with the endpoint macros.
			 */
			#define ENDPOINT_CONTROLEP                      0

			/* Private Interface - For use in library only: */
				/* Inline Functions: */
					static inline uint8_t Endpoint_BytesToEPSizeMask(const uint16_t Bytes) ATTR_WARN_UNUSED_RESULT ATTR_CONST
																																								 ATTR_ALWAYS_INLINE;
					static inline uint8_t Endpoint_BytesToEPSizeMask(const uint16_t Bytes)
					{
						uint8_t  MaskVal    = 0;
						uint16_t CheckBytes = 8;

						while (CheckBytes < Bytes)
						{
							MaskVal++;
							CheckBytes <<= 1;
						}

						return (MaskVal << EPSIZE0);
					}

				/* Defines: */
					/** \name Endpoint Direction Masks */
					//@{
					/** Endpoint direction mask, for masking against endpoint addresses to retrieve the endpoint's
					 *  direction for comparing with the \c ENDPOINT_DIR_* masks.
					 */
					#define ENDPOINT_DIR_MASK                  0x80

					/** Endpoint address direction mask for an OUT direction (Host to Device) endpoint. This may be ORed with
					 *  the index of the address within a device to obtain the full endpoint address.
					 */
					#define ENDPOINT_DIR_OUT                   0x00

					/** Endpoint address direction mask for an IN direction (Device to Host) endpoint. This may be ORed with
					 *  the index of the address within a device to obtain the full endpoint address.
					 */
					#define ENDPOINT_DIR_IN                    0x80
					//@}

					/** \name Pipe Direction Masks */
					//@{
					/** Pipe direction mask, for masking against pipe addresses to retrieve the pipe's
					 *  direction for comparing with the \c PIPE_DIR_* masks.
					 */
					#define PIPE_DIR_MASK                      0x80

					/** Endpoint address direction mask for an OUT direction (Host to Device) endpoint. This may be ORed with
					 *  the index of the address within a device to obtain the full endpoint address.
					 */
					#define PIPE_DIR_OUT                       0x00

					/** Endpoint address direction mask for an IN direction (Device to Host) endpoint. This may be ORed with
					 *  the index of the address within a device to obtain the full endpoint address.
					 */
					#define PIPE_DIR_IN                        0x80
					//@}

					/** \name Endpoint/Pipe Type Masks */
					//@{
					/** Mask for determining the type of an endpoint from an endpoint descriptor. This should then be compared
					 *  with the \c EP_TYPE_* masks to determine the exact type of the endpoint.
					 */
					#define EP_TYPE_MASK                       0x03

					/** Mask for a CONTROL type endpoint or pipe.
					 *
					 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for endpoint/pipe functions.
					 */
					#define EP_TYPE_CONTROL                    0x00

					/** Mask for an ISOCHRONOUS type endpoint or pipe.
					 *
					 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for endpoint/pipe functions.
					 */
					#define EP_TYPE_ISOCHRONOUS                0x01

					/** Mask for a BULK type endpoint or pipe.
					 *
					 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for endpoint/pipe functions.
					 */
					#define EP_TYPE_BULK                       0x02

					/** Mask for an INTERRUPT type endpoint or pipe.
					 *
					 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for endpoint/pipe functions.
					 */
					#define EP_TYPE_INTERRUPT                  0x03
					//@}

			/* Public Interface - May be used in end-application: */
				/* Macros: */
					#if !defined(CONTROL_ONLY_DEVICE)
						#if defined(USB_SERIES_4_AVR) || defined(USB_SERIES_6_AVR)
							/** Total number of endpoints (including the default control endpoint at address 0) which may
							 *  be used in the device. Different USB AVR models support different amounts of endpoints,
							 *  this value reflects the maximum number of endpoints for the currently selected AVR model.
							 */
							#define ENDPOINT_TOTAL_ENDPOINTS        7
						#else
							#define ENDPOINT_TOTAL_ENDPOINTS        5
						#endif
					#else
						#define ENDPOINT_TOTAL_ENDPOINTS            1
					#endif

				/* Enums: */
					/** Enum for the possible error return codes of the \ref Endpoint_WaitUntilReady() function.
					 *
					 *  \ingroup Group_EndpointRW_AVR8
					 */
					enum Endpoint_WaitUntilReady_ErrorCodes_t
					{
						ENDPOINT_READYWAIT_NoError                 = 0, /**< Endpoint is ready for next packet, no error. */
						ENDPOINT_READYWAIT_EndpointStalled         = 1, /**< The endpoint was stalled during the stream
						                                                 *   transfer by the host or device.
						                                                 */
						ENDPOINT_READYWAIT_DeviceDisconnected      = 2,	/**< Device was disconnected from the host while
						                                                 *   waiting for the endpoint to become ready.
						                                                 */
						ENDPOINT_READYWAIT_BusSuspended            = 3, /**< The USB bus has been suspended by the host and
						                                                 *   no USB endpoint traffic can occur until the bus
						                                                 *   has resumed.
						                                                 */
						ENDPOINT_READYWAIT_Timeout                 = 4, /**< The host failed to accept or send the next packet
						                                                 *   within the software timeout period set by the
						                                                 *   \ref USB_STREAM_TIMEOUT_MS macro.
						                                                 */
					};

				/* Inline Functions: */
					/** Indicates the number of bytes currently stored in the current endpoint's selected bank.
					 *
					 *  \ingroup Group_EndpointRW_AVR8
					 *
					 *  \return Total number of bytes in the currently selected Endpoint's FIFO buffer.
					 */
					static inline uint16_t Endpoint_BytesInEndpoint(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint16_t Endpoint_BytesInEndpoint(void)
					{
						#if defined(USB_SERIES_6_AVR)
							return UEBCX;
						#elif defined(USB_SERIES_4_AVR)
							return (((uint16_t)UEBCHX << 8) | UEBCLX);
						#elif defined(USB_SERIES_2_AVR)
							return UEBCLX;
						#endif
					}

					/** Determines the currently selected endpoint's direction.
					 *
					 *  \return The currently selected endpoint's direction, as a \c ENDPOINT_DIR_* mask.
					 */
					static inline uint8_t Endpoint_GetEndpointDirection(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint8_t Endpoint_GetEndpointDirection(void)
					{
						return (UECFG0X & (1 << EPDIR)) ? ENDPOINT_DIR_IN : ENDPOINT_DIR_OUT;
					}

					/** Get the endpoint address of the currently selected endpoint. This is typically used to save
					 *  the currently selected endpoint so that it can be restored after another endpoint has been
					 *  manipulated.
					 *
					 *  \return Index of the currently selected endpoint.
					 */
					static inline uint8_t Endpoint_GetCurrentEndpoint(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint8_t Endpoint_GetCurrentEndpoint(void)
					{
						#if !defined(CONTROL_ONLY_DEVICE)
							return ((UENUM & ENDPOINT_EPNUM_MASK) | Endpoint_GetEndpointDirection());
						#else
							return ENDPOINT_CONTROLEP;
						#endif
					}

					/** Selects the given endpoint address.
					 *
					 *  Any endpoint operations which do not require the endpoint address to be indicated will operate on
					 *  the currently selected endpoint.
					 *
					 *  \param[in] Address Endpoint address to select.
					 */
					static inline void Endpoint_SelectEndpoint(const uint8_t Address) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_SelectEndpoint(const uint8_t Address)
					{
						#if !defined(CONTROL_ONLY_DEVICE)
							UENUM = (Address & ENDPOINT_EPNUM_MASK);
						#endif
					}

					/** Resets the endpoint bank FIFO. This clears all the endpoint banks and resets the USB controller's
					 *  data In and Out pointers to the bank's contents.
					 *
					 *  \param[in] Address  Endpoint address whose FIFO buffers are to be reset.
					 */
					static inline void Endpoint_ResetEndpoint(const uint8_t Address) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ResetEndpoint(const uint8_t Address)
					{
						UERST = (1 << (Address & ENDPOINT_EPNUM_MASK));
						UERST = 0;
					}

					/** Enables the currently selected endpoint so that data can be sent and received through it to
					 *  and from a host.
					 *
					 *  \note Endpoints must first be configured properly via \ref Endpoint_ConfigureEndpoint().
					 */
					static inline void Endpoint_EnableEndpoint(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_EnableEndpoint(void)
					{
						UECONX |= (1 << EPEN);
					}

					/** Disables the currently selected endpoint so that data cannot be sent and received through it
					 *  to and from a host.
					 */
					static inline void Endpoint_DisableEndpoint(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_DisableEndpoint(void)
					{
						UECONX &= ~(1 << EPEN);
					}

					/** Determines if the currently selected endpoint is enabled, but not necessarily configured.
					 *
					 * \return Boolean \c true if the currently selected endpoint is enabled, \c false otherwise.
					 */
					static inline bool Endpoint_IsEnabled(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsEnabled(void)
					{
						return ((UECONX & (1 << EPEN)) ? true : false);
					}

					/** Retrieves the number of busy banks in the currently selected endpoint, which have been queued for
					 *  transmission via the \ref Endpoint_ClearIN() command, or are awaiting acknowledgment via the
					 *  \ref Endpoint_ClearOUT() command.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \return Total number of busy banks in the selected endpoint.
					 */
					static inline uint8_t Endpoint_GetBusyBanks(void) ATTR_ALWAYS_INLINE ATTR_WARN_UNUSED_RESULT;
					static inline uint8_t Endpoint_GetBusyBanks(void)
					{
						return (UESTA0X & (0x03 << NBUSYBK0));
					}

					/** Aborts all pending IN transactions on the currently selected endpoint, once the bank
					 *  has been queued for transmission to the host via \ref Endpoint_ClearIN(). This function
					 *  will terminate all queued transactions, resetting the endpoint banks ready for a new
					 *  packet.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 */
					static inline void Endpoint_AbortPendingIN(void)
					{
						while (Endpoint_GetBusyBanks() != 0)
						{
							UEINTX |= (1 << RXOUTI);
							while (UEINTX & (1 << RXOUTI));
						}
					}

					/** Determines if the currently selected endpoint may be read from (if data is waiting in the endpoint
					 *  bank and the endpoint is an OUT direction, or if the bank is not yet full if the endpoint is an IN
					 *  direction). This function will return false if an error has occurred in the endpoint, if the endpoint
					 *  is an OUT direction and no packet (or an empty packet) has been received, or if the endpoint is an IN
					 *  direction and the endpoint bank is full.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \return Boolean \c true if the currently selected endpoint may be read from or written to, depending
					 *          on its direction.
					 */
					static inline bool Endpoint_IsReadWriteAllowed(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsReadWriteAllowed(void)
					{
						return ((UEINTX & (1 << RWAL)) ? true : false);
					}

					/** Determines if the currently selected endpoint is configured.
					 *
					 *  \return Boolean \c true if the currently selected endpoint has been configured, \c false otherwise.
					 */
					static inline bool Endpoint_IsConfigured(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsConfigured(void)
					{
						return ((UESTA0X & (1 << CFGOK)) ? true : false);
					}

					/** Returns a mask indicating which INTERRUPT type endpoints have interrupted - i.e. their
					 *  interrupt duration has elapsed. Which endpoints have interrupted can be determined by
					 *  masking the return value against <tt>(1 << <i>{Endpoint Number}</i>)</tt>.
					 *
					 *  \return Mask whose bits indicate which endpoints have interrupted.
					 */
					static inline uint8_t Endpoint_GetEndpointInterrupts(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint8_t Endpoint_GetEndpointInterrupts(void)
					{
						return UEINT;
					}

					/** Determines if the specified endpoint number has interrupted (valid only for INTERRUPT type
					 *  endpoints).
					 *
					 *  \param[in] Address  Address of the endpoint whose interrupt flag should be tested.
					 *
					 *  \return Boolean \c true if the specified endpoint has interrupted, \c false otherwise.
					 */
					static inline bool Endpoint_HasEndpointInterrupted(const uint8_t Address) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_HasEndpointInterrupted(const uint8_t Address)
					{
						return ((Endpoint_GetEndpointInterrupts() & (1 << (Address & ENDPOINT_EPNUM_MASK))) ? true : false);
					}

					/** Determines if the selected IN endpoint is ready for a new packet to be sent to the host.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \return Boolean \c true if the current endpoint is ready for an IN packet, \c false otherwise.
					 */
					static inline bool Endpoint_IsINReady(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsINReady(void)
					{
						return ((UEINTX & (1 << TXINI)) ? true : false);
					}

					/** Determines if the selected OUT endpoint has received new packet from the host.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \return Boolean \c true if current endpoint is has received an OUT packet, \c false otherwise.
					 */
					static inline bool Endpoint_IsOUTReceived(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsOUTReceived(void)
					{
						return ((UEINTX & (1 << RXOUTI)) ? true : false);
					}

					/** Determines if the current CONTROL type endpoint has received a SETUP packet.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \return Boolean \c true if the selected endpoint has received a SETUP packet, \c false otherwise.
					 */
					static inline bool Endpoint_IsSETUPReceived(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsSETUPReceived(void)
					{
						return ((UEINTX & (1 << RXSTPI)) ? true : false);
					}

					/** Clears a received SETUP packet on the currently selected CONTROL type endpoint, freeing up the
					 *  endpoint for the next packet.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \note This is not applicable for non CONTROL type endpoints.
					 */
					static inline void Endpoint_ClearSETUP(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ClearSETUP(void)
					{
						UEINTX &= ~(1 << RXSTPI);
					}

					/** Sends an IN packet to the host on the currently selected endpoint, freeing up the endpoint for the
					 *  next packet and switching to the alternative endpoint bank if double banked.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 */
					static inline void Endpoint_ClearIN(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ClearIN(void)
					{
						#if !defined(CONTROL_ONLY_DEVICE)
							UEINTX &= ~((1 << TXINI) | (1 << FIFOCON));
						#else
							UEINTX &= ~(1 << TXINI);
						#endif
					}

					/** Acknowledges an OUT packet to the host on the currently selected endpoint, freeing up the endpoint
					 *  for the next packet and switching to the alternative endpoint bank if double banked.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 */
					static inline void Endpoint_ClearOUT(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ClearOUT(void)
					{
						#if !defined(CONTROL_ONLY_DEVICE)
							UEINTX &= ~((1 << RXOUTI) | (1 << FIFOCON));
						#else
							UEINTX &= ~(1 << RXOUTI);
						#endif
					}

					/** Stalls the current endpoint, indicating to the host that a logical problem occurred with the
					 *  indicated endpoint and that the current transfer sequence should be aborted. This provides a
					 *  way for devices to indicate invalid commands to the host so that the current transfer can be
					 *  aborted and the host can begin its own recovery sequence.
					 *
					 *  The currently selected endpoint remains stalled until either the \ref Endpoint_ClearStall() macro
					 *  is called, or the host issues a CLEAR FEATURE request to the device for the currently selected
					 *  endpoint.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 */
					static inline void Endpoint_StallTransaction(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_StallTransaction(void)
					{
						UECONX |= (1 << STALLRQ);
					}

					/** Clears the STALL condition on the currently selected endpoint.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 */
					static inline void Endpoint_ClearStall(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ClearStall(void)
					{
						UECONX |= (1 << STALLRQC);
					}

					/** Determines if the currently selected endpoint is stalled, \c false otherwise.
					 *
					 *  \ingroup Group_EndpointPacketManagement_AVR8
					 *
					 *  \return Boolean \c true if the currently selected endpoint is stalled, \c false otherwise.
					 */
					static inline bool Endpoint_IsStalled(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline bool Endpoint_IsStalled(void)
					{
						return ((UECONX & (1 << STALLRQ)) ? true : false);
					}

					/** Resets the data toggle of the currently selected endpoint. */
					static inline void Endpoint_ResetDataToggle(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ResetDataToggle(void)
					{
						UECONX |= (1 << RSTDT);
					}

					/** Sets the direction of the currently selected endpoint.
					 *
					 *  \param[in] DirectionMask  New endpoint direction, as a \c ENDPOINT_DIR_* mask.
					 */
					static inline void Endpoint_SetEndpointDirection(const uint8_t DirectionMask) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_SetEndpointDirection(const uint8_t DirectionMask)
					{
						UECFG0X = ((UECFG0X & ~(1 << EPDIR)) | (DirectionMask ? (1 << EPDIR) : 0));
					}

					/** Reads one byte from the currently selected endpoint's bank, for OUT direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \return Next byte in the currently selected endpoint's FIFO buffer.
					 */
					static inline uint8_t Endpoint_Read_8(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint8_t Endpoint_Read_8(void)
					{
						return UEDATX;
					}

					/** Writes one byte to the currently selected endpoint's bank, for IN direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \param[in] Data  Data to write into the the currently selected endpoint's FIFO buffer.
					 */
					static inline void Endpoint_Write_8(const uint8_t Data) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Write_8(const uint8_t Data)
					{
						UEDATX = Data;
					}

					/** Discards one byte from the currently selected endpoint's bank, for OUT direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 */
					static inline void Endpoint_Discard_8(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Discard_8(void)
					{
						uint8_t Dummy;

						Dummy = UEDATX;

						(void)Dummy;
					}

					/** Reads two bytes from the currently selected endpoint's bank in little endian format, for OUT
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \return Next two bytes in the currently selected endpoint's FIFO buffer.
					 */
					static inline uint16_t Endpoint_Read_16_LE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint16_t Endpoint_Read_16_LE(void)
					{
						union
						{
							uint16_t Value;
							uint8_t  Bytes[2];
						} Data;

						Data.Bytes[0] = UEDATX;
						Data.Bytes[1] = UEDATX;

						return Data.Value;
					}

					/** Reads two bytes from the currently selected endpoint's bank in big endian format, for OUT
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \return Next two bytes in the currently selected endpoint's FIFO buffer.
					 */
					static inline uint16_t Endpoint_Read_16_BE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint16_t Endpoint_Read_16_BE(void)
					{
						union
						{
							uint16_t Value;
							uint8_t  Bytes[2];
						} Data;

						Data.Bytes[1] = UEDATX;
						Data.Bytes[0] = UEDATX;

						return Data.Value;
					}

					/** Writes two bytes to the currently selected endpoint's bank in little endian format, for IN
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \param[in] Data  Data to write to the currently selected endpoint's FIFO buffer.
					 */
					static inline void Endpoint_Write_16_LE(const uint16_t Data) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Write_16_LE(const uint16_t Data)
					{
						UEDATX = (Data & 0xFF);
						UEDATX = (Data >> 8);
					}

					/** Writes two bytes to the currently selected endpoint's bank in big endian format, for IN
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \param[in] Data  Data to write to the currently selected endpoint's FIFO buffer.
					 */
					static inline void Endpoint_Write_16_BE(const uint16_t Data) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Write_16_BE(const uint16_t Data)
					{
						UEDATX = (Data >> 8);
						UEDATX = (Data & 0xFF);
					}

					/** Discards two bytes from the currently selected endpoint's bank, for OUT direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 */
					static inline void Endpoint_Discard_16(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Discard_16(void)
					{
						uint8_t Dummy;

						Dummy = UEDATX;
						Dummy = UEDATX;

						(void)Dummy;
					}

					/** Reads four bytes from the currently selected endpoint's bank in little endian format, for OUT
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \return Next four bytes in the currently selected endpoint's FIFO buffer.
					 */
					static inline uint32_t Endpoint_Read_32_LE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint32_t Endpoint_Read_32_LE(void)
					{
						union
						{
							uint32_t Value;
							uint8_t  Bytes[4];
						} Data;

						Data.Bytes[0] = UEDATX;
						Data.Bytes[1] = UEDATX;
						Data.Bytes[2] = UEDATX;
						Data.Bytes[3] = UEDATX;

						return Data.Value;
					}

					/** Reads four bytes from the currently selected endpoint's bank in big endian format, for OUT
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \return Next four bytes in the currently selected endpoint's FIFO buffer.
					 */
					static inline uint32_t Endpoint_Read_32_BE(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
					static inline uint32_t Endpoint_Read_32_BE(void)
					{
						union
						{
							uint32_t Value;
							uint8_t  Bytes[4];
						} Data;

						Data.Bytes[3] = UEDATX;
						Data.Bytes[2] = UEDATX;
						Data.Bytes[1] = UEDATX;
						Data.Bytes[0] = UEDATX;

						return Data.Value;
					}

					/** Writes four bytes to the currently selected endpoint's bank in little endian format, for IN
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \param[in] Data  Data to write to the currently selected endpoint's FIFO buffer.
					 */
					static inline void Endpoint_Write_32_LE(const uint32_t Data) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Write_32_LE(const uint32_t Data)
					{
						UEDATX = (Data &  0xFF);
						UEDATX = (Data >> 8);
						UEDATX = (Data >> 16);
						UEDATX = (Data >> 24);
					}

					/** Writes four bytes to the currently selected endpoint's bank in big endian format, for IN
					 *  direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 *
					 *  \param[in] Data  Data to write to the currently selected endpoint's FIFO buffer.
					 */
					static inline void Endpoint_Write_32_BE(const uint32_t Data) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Write_32_BE(const uint32_t Data)
					{
						UEDATX = (Data >> 24);
						UEDATX = (Data >> 16);
						UEDATX = (Data >> 8);
						UEDATX = (Data &  0xFF);
					}

					/** Discards four bytes from the currently selected endpoint's bank, for OUT direction endpoints.
					 *
					 *  \ingroup Group_EndpointPrimitiveRW_AVR8
					 */
					static inline void Endpoint_Discard_32(void) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_Discard_32(void)
					{
						uint8_t Dummy;

						Dummy = UEDATX;
						Dummy = UEDATX;
						Dummy = UEDATX;
						Dummy = UEDATX;

						(void)Dummy;
					}

					/** Configures the specified endpoint address with the given endpoint type, bank size and number of hardware
					 *  banks. Once configured, the endpoint may be read from or written to, depending on its direction.
					 *
					 *  \param[in] Address    Endpoint address to configure.
					 *
					 *  \param[in] Type       Type of endpoint to configure, a \c EP_TYPE_* mask. Not all endpoint types
					 *                        are available on Low Speed USB devices - refer to the USB 2.0 specification.
					 *
					 *  \param[in] Size       Size of the endpoint's bank, where packets are stored before they are transmitted
					 *                        to the USB host, or after they have been received from the USB host (depending on
					 *                        the endpoint's data direction). The bank size must indicate the maximum packet size
					 *                        that the endpoint can handle.
					 *
					 *  \param[in] Banks      Number of banks to use for the endpoint being configured.
					 *
					 *  \attention Endpoints <b>must</b> be configured in
					 *             ascending order, or bank corruption will occur.
					 *
					 *  \note Different endpoints may have different maximum packet sizes based on the endpoint's index - please
					 *        refer to the chosen microcontroller model's datasheet to determine the maximum bank size for each endpoint.
					 *        \n\n
					 *
					 *  \note The default control endpoint should not be manually configured by the user application, as
					 *        it is automatically configured by the library internally.
					 *        \n\n
					 *
					 *  \note This routine will automatically select the specified endpoint upon success. Upon failure, the endpoint
					 *        which failed to reconfigure correctly will be selected.
					 *
					 *  \return Boolean \c true if the configuration succeeded, \c false otherwise.
					 */
					static inline void Endpoint_ConfigureEndpoint(const uint8_t Address,
																												const uint8_t Type,
																												const uint16_t Size,
																												const uint8_t Banks) ATTR_ALWAYS_INLINE;
					static inline void Endpoint_ConfigureEndpoint(const uint8_t Address,
																												const uint8_t Type,
																												const uint16_t Size,
																												const uint8_t Banks)
					{
						uint8_t Number = (Address & ENDPOINT_EPNUM_MASK);

						if (Number >= ENDPOINT_TOTAL_ENDPOINTS)
							return;

						const uint8_t UECFG0XData = ((Type << EPTYPE0) | ((Address & ENDPOINT_DIR_IN) ? (1 << EPDIR) : 0));
						const uint8_t UECFG1XData = ((1 << ALLOC) | ((Banks > 1) ? (1 << EPBK0) : 0) | Endpoint_BytesToEPSizeMask(Size));

						Endpoint_SelectEndpoint(Number);
						Endpoint_EnableEndpoint();

						UECFG1X = 0;
						UECFG0X = UECFG0XData;
						UECFG1X = UECFG1XData;

						return;
					}

				/* External Variables: */
				/** Global indicating the maximum packet size of the default control endpoint located at address
				 *  0 in the device. This value is set to the value indicated in the device descriptor in the user
				 *  project once the USB interface is initialized into device mode.
				 */
				#define FIXED_CONTROL_ENDPOINT_SIZE      64

					static inline void Endpoint_ClearStatusStageDeviceToHost(void);
					static inline void Endpoint_ClearStatusStageDeviceToHost(void)
					{
						while (!(Endpoint_IsOUTReceived()));

						Endpoint_ClearOUT();
					}
					static inline void Endpoint_ClearStatusStageHostToDevice(void);
					static inline void Endpoint_ClearStatusStageHostToDevice(void)
					{
						while (!(Endpoint_IsINReady()));

						Endpoint_ClearIN();
					}

					// TODO remove
					// static inline void Endpoint_ClearStatusStage(void);
					// static inline void Endpoint_ClearStatusStage(void)
					// {
					// 	if (USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST)
					// 	{
					// 		Endpoint_ClearStatusStageDeviceToHost();
					// 	}
					// 	else
					// 	{
					// 		Endpoint_ClearStatusStageHostToDevice();
					// 	}
					// }

					/* Public Interface - May be used in end-application: */
							/** Writes the given number of bytes to the CONTROL type endpoint from the given buffer in little endian,
							 *  sending full packets to the host as needed. The host OUT acknowledgement is not automatically cleared
							 *  in both failure and success states; the user is responsible for manually clearing the status OUT packet
							 *  to finalize the transfer's status stage via the \ref Endpoint_ClearOUT() macro.
							 *
							 *  \note This function automatically sends the last packet in the data stage of the transaction; when the
							 *        function returns, the user is responsible for clearing the <b>status</b> stage of the transaction.
							 *        Note that the status stage packet is sent or received in the opposite direction of the data flow.
							 *        \n\n
							 *
							 *  \note This routine should only be used on CONTROL type endpoints.
							 *
							 *  \warning Unlike the standard stream read/write commands, the control stream commands cannot be chained
							 *           together; i.e. the entire stream data must be read or written at the one time.
							 *
							 *  \param[in] Buffer  Pointer to the source data buffer to read from.
							 *  \param[in] Length  Number of bytes to read for the currently selected endpoint into the buffer.
				 		 	 */
							void Endpoint_Write_Control_Stream_LE(const void* const Buffer,
																											 uint16_t Length) ATTR_NON_NULL_PTR_ARG(1);


	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif

/** @} */
