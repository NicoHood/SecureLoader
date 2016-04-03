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


; Trampolines to actual API implementations if the target address is outside the
; range of a rjmp instruction (can happen with large bootloader sections)
.section .apitable_trampolines, "ax"
.global BootloaderAPI_Trampolines


; BootloaderPROGMEMKey 32 byte, 256 bit
.byte 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe
.byte	0x2b,	0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81
.byte	0x1f, 0x35, 0x2c, 0x07, 0x3b,	0x61, 0x08, 0xd7
.byte	0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4

; useHardwareButton
.byte 0x00

BootloaderAPI_Trampolines:

  .word 0xAAAA ; Signature for a LUFA class bootloader

	BootloaderAPI_aes256_init_ecb_Trampoline:
		jmp aes256_init_ecb
	BootloaderAPI_aes256_encrypt_ecb_Trampoline:
		jmp aes256_decrypt_ecb
	BootloaderAPI_aes256_decrypt_ecb_Trampoline:
		ret
	BootloaderAPI_UNUSED4:
		ret
	BootloaderAPI_UNUSED5:
		ret



; API function jump table
.section .apitable_jumptable, "ax"
.global BootloaderAPI_JumpTable
BootloaderAPI_JumpTable:

	rjmp BootloaderAPI_aes256_init_ecb_Trampoline
	rjmp BootloaderAPI_aes256_encrypt_ecb_Trampoline
	rjmp BootloaderAPI_aes256_decrypt_ecb_Trampoline
	rjmp BootloaderAPI_UNUSED4 ; UNUSED ENTRY 4
	rjmp BootloaderAPI_UNUSED5 ; UNUSED ENTRY 5


; Bootloader table signatures and information
.section .apitable_signatures, "ax"
.global BootloaderAPI_Signatures
BootloaderAPI_Signatures:

	.long BOOT_START_ADDR ; Start address of the bootloader
	.word 0xDF00 ; Signature for the CDC class bootloader
	.word 0xDCFB ; Signature for a LUFA class bootloader
