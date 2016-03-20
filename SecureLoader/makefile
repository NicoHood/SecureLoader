#
#             LUFA Library
#     Copyright (C) Dean Camera, 2016.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = LEONARDO
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = BootloaderHID
SRC          = $(TARGET).c Descriptors.c BootloaderAPI.c $(LIB_SRC) $(LUFA_SRC_USB)
LUFA_PATH    = ../lufa/LUFA
CC_FLAGS     = $(OPTIONS) -DUSE_LUFA_CONFIG_HEADER -IConfig/ -DBOOT_START_ADDR=$(BOOT_START_OFFSET) -flto -fuse-linker-plugin
LD_FLAGS     = -Wl,--section-start=.text=$(BOOT_START_OFFSET) $(BOOT_API_LD_FLAGS) -flto -fuse-linker-plugin

# Flash size and bootloader section sizes of the target, in KB. These must
# match the target's total FLASH size and the bootloader size set in the
# device's fuses.
FLASH_SIZE_KB        := 32
BOOT_SECTION_SIZE_KB := 4

# Gcc settings
COMPILER_PATH =../AVR-Development-Environment-Script/bin/bin/
C_INCLUDE_PATH=./lib/

OPTIONS += -DVENDORID=0x03EB
OPTIONS += -DPRODUCTID=0x2067
OPTIONS += -DBAUD=115200

#SRC += BootloaderAPITable.S

# Additional source files from libraries
LIB_SRC += src/AES/aes.c
LIB_SRC += src/AES/aes256_ctr.c
LIB_SRC += src/serial/serial1.c

# Bootloader address calculation formulas
# Do not modify these macros, but rather modify the dependent values above.
CALC_ADDRESS_IN_HEX   = $(shell printf "0x%X" $$(( $(1) )) )
BOOT_START_OFFSET     = $(call CALC_ADDRESS_IN_HEX, ($(FLASH_SIZE_KB) - $(BOOT_SECTION_SIZE_KB)) * 1024 )
BOOT_SEC_OFFSET       = $(call CALC_ADDRESS_IN_HEX, ($(FLASH_SIZE_KB) * 1024) - ($(strip $(1))) )

# Bootloader linker section flags for relocating the API table sections to
# known FLASH addresses - these should not normally be user-edited.
BOOT_SECTION_LD_FLAG  = -Wl,--section-start=$(strip $(1))=$(call BOOT_SEC_OFFSET, $(3)) -Wl,--undefined=$(strip $(2))
BOOT_API_LD_FLAGS     = $(call BOOT_SECTION_LD_FLAG, .apitable_trampolines, BootloaderAPI_Trampolines, 96)
BOOT_API_LD_FLAGS    += $(call BOOT_SECTION_LD_FLAG, .apitable_jumptable,   BootloaderAPI_JumpTable,   32)
BOOT_API_LD_FLAGS    += $(call BOOT_SECTION_LD_FLAG, .apitable_signatures,  BootloaderAPI_Signatures,  8)

# Avrdude settings
AVRDUDE_PORT       = /dev/ttyACM1
AVRDUDE_PROGRAMMER = stk500v1
AVRDUDE_FLAGS      = -b 19200
#	avrdude -patmega32u4 -cstk500v1 -P/dev/ttyACM1 -b19200 -e -Ulock:w:0x3F:m -Uefuse:w:0xCB:m -Uhfuse:w:0xD8:m -Ulfuse:w:0xFF:m
#	avrdude -patmega32u4 -cstk500v1 -P/dev/ttyACM1 -b19200 -Uflash:w:./$(TARGET).hex:i -Ulock:w:0x2F:m


# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk

noreboot:
	cd HostLoaderApp && $(MAKE) && cd ..
	./HostLoaderApp/hid_bootloader_cli -mmcu=atmega32u4 -v -n ./HostLoaderApp/blink/blink1000.hex

upload:
	cd HostLoaderApp && $(MAKE) && cd ..
	./HostLoaderApp/hid_bootloader_cli -mmcu=atmega32u4 -v ./HostLoaderApp/blink/blink1000.hex

uploadfast:
	./HostLoaderApp/hid_bootloader_cli -mmcu=atmega32u4 -v ./HostLoaderApp/blink/blink100.hex
