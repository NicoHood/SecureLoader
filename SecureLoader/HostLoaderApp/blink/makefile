#
#            DMBS Build System
#     Released into the public domain.
#
#  dean [at] fourwalledcubicle [dot] com
#        www.fourwalledcubicle.com
#

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
F_CPU        = 16000000
OPTIMIZATION = s
TARGET       = main
SRC          = $(TARGET).c
CC_FLAGS     =
LD_FLAGS     =

# Avrdude settings
AVRDUDE_PORT           = /dev/ttyACM1
ifeq ($(MCU), atmega2560)
    AVRDUDE_PROGRAMMER = wiring
    AVRDUDE_FLAGS      = -b 115200 -D
else ifeq ($(MCU), atmega32u4)
    AVRDUDE_PROGRAMMER = stk500v1
    AVRDUDE_FLAGS      = -b19200
else
    AVRDUDE_PROGRAMMER = arduino
    AVRDUDE_FLAGS      = -b 115200
endif

# Default target
all:

# Include DMBS build script makefiles
DMBS_PATH   ?= ./DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk
