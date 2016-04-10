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
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = SecureLoader
SRC          = $(TARGET).c BootloaderAPI.c $(LIBOBJS) $(LUFA_SRC_USB)
DMBS_PATH    = ../dmbs/DMBS
CC_FLAGS     = $(OPTIONS) -DUSE_LUFA_CONFIG_HEADER -IConfig/ -DBOOT_START_ADDR=$(BOOT_START_OFFSET)
LD_FLAGS     = -Wl,--section-start=.text=$(BOOT_START_OFFSET) $(BOOT_API_LD_FLAGS)

# Flash size and bootloader section sizes of the target, in KB. These must
# match the target's total FLASH size and the bootloader size set in the
# device's fuses.
FLASH_SIZE_KB			:= 32
BOOT_SECTION_SIZE_KB	:= 4

# Special optimization (most important at the top)
# COMPILER_PATH =../AVR-Development-Environment-Script/bin/bin/
# CC_FLAGS  += -flto -fuse-linker-plugin
# LD_FLAGS  += -flto -fuse-linker-plugin
CC_FLAGS  += -fno-split-wide-types
CC_FLAGS  += -fno-tree-scev-cprop

OPTIONS += -DVENDORID=0x03EB
OPTIONS += -DPRODUCTID=0x2067
OPTIONS += -DBAUD=115200 # TODO remove
OPTIONS += -DSTARTUP_TABLES
OPTIONS += -DF_USB=$(F_USB)

SRC += BootloaderAPITable.S

# Avrdude settings
AVRDUDE_PORT       = /dev/ttyACM0
AVRDUDE_PROGRAMMER = stk500v1
AVRDUDE_FLAGS      = -b 19200
AVRDUDE_HFUSE      = 0xD8
AVRDUDE_EFUSE      = 0xCB
AVRDUDE_LFUSE      = 0xFF
AVRDUDE_LOCK       = 0x1C

# Some helpers to get all the .c and .h files.
# $(TARGET) is included inside $(OBJECTS).
LIBDIRS := $(addprefix ./, AES SERIAL USB)
LIBOBJS := $(foreach lib, $(LIBDIRS), $(wildcard $(lib)/*.c))
OBJECTS := $(wildcard ./*.c)

# Bootloader address calculation formulas
# Do not modify these macros, but rather modify the dependent values above.
CALC_ADDRESS_IN_HEX   = $(shell printf "0x%X" $$(( $(1) )) )
BOOT_START_OFFSET     = $(call CALC_ADDRESS_IN_HEX, ($(FLASH_SIZE_KB) - $(BOOT_SECTION_SIZE_KB)) * 1024 )
BOOT_SEC_OFFSET       = $(call CALC_ADDRESS_IN_HEX, ($(FLASH_SIZE_KB) * 1024) - ($(strip $(1))) )

# Bootloader linker section flags for relocating the API table sections to
# known FLASH addresses - these should not normally be user-edited.
BOOT_SECTION_LD_FLAG  = -Wl,--section-start=$(strip $(1))=$(call BOOT_SEC_OFFSET, $(3)) -Wl,--undefined=$(strip $(2))
BOOT_API_LD_FLAGS    += $(call BOOT_SECTION_LD_FLAG, .apitable_sbs, BootloaderAPI_sbs, 256)
BOOT_API_LD_FLAGS    += $(call BOOT_SECTION_LD_FLAG, .apitable_bootloader_key, BootloaderAPI_bootloader_key, 160)
BOOT_API_LD_FLAGS    += $(call BOOT_SECTION_LD_FLAG, .apitable_functions, BootloaderAPI_functions, 128)
BOOT_API_LD_FLAGS    += $(call BOOT_SECTION_LD_FLAG, .apitable_jumptable, BootloaderAPI_JumpTable,   4)


# Default target
all:

# Include DMBS build script makefiles
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk

cli:
	cd HostLoaderApp && $(MAKE) clean && $(MAKE) && cd ..

noreboot: cli
	./HostLoaderApp/SecureLoaderCli -v -n -w ./HostLoaderApp/blink/blink100.hex

upload: cli
	./HostLoaderApp/SecureLoaderCli -v -w ./HostLoaderApp/blink/blink1000.hex

uploadfast: cli
	./HostLoaderApp/SecureLoaderCli -v -w ./HostLoaderApp/blink/blink100.hex
