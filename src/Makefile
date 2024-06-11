##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

.SUFFIXES:            # Delete the default suffixes
MAKEFLAGS += --no-builtin-rules

ifeq ($(OS), Windows_NT)
  PYTHON = python
else ifeq ($(shell which python3),)
  PYTHON = python
else
  PYTHON = python3
endif

# Set CPU target (default STM32F4XX)
export STM32F4XX ?= 1

# Set to 1 HYDRAFW_NFC to include HydraNFC extension support
export HYDRAFW_NFC ?= 1
export HYDRAFW_DEBUG ?= 0
export FW_REVISION := $(shell build-scripts/hydrafw-revision)

HYDRAFW_OPTS =

ifeq ($(HYDRAFW_NFC),1)
HYDRAFW_OPTS += -DHYDRANFC
endif

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -fomit-frame-pointer -falign-functions=16 -std=gnu89 --specs=nosys.specs
endif

ifeq ($(HYDRAFW_DEBUG),1)
  HYDRAFW_OPTS += -DMAKE_DEBUG
  # Host Debug is either async trace via SWO or Semihosting
  # Leave disabled if using neither.
  HYDRAFW_OPTS += -DUSE_HOST_DEBUG
  USE_OPT += -ggdb
  USE_OPT += -fstack-usage
  USE_OPT += -Og
  USE_OPT += -Wshadow
  USE_OPT += -Wformat=2
  USE_LTO = no
else
  USE_OPT += -Os
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = 
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT = 
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(V),1)
  USE_VERBOSE_COMPILE = yes
endif

# Also set directives to use with commands depending on verbosity status.
ifeq ($(USE_VERBOSE_COMPILE),yes)
  # if verbose build then print out the full commands without description
  OUT_LOG = @\#
  OUT_CMD =
else
  # if silent build then print out only short description for commands
  OUT_LOG = @
  OUT_CMD = @
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
# Warning if set to yes chconf.h & halconf.h shall be in same directory as this file
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x400
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

# Enables the use of FPU (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = hard
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = hydrafw

# Imported source files and paths
CHIBIOS = chibios

# Startup files.
ifeq ($(STM32F4XX),1)
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f4xx.mk
endif

# HAL-OSAL files (optional).
include $(CHIBIOS)/os/hal/hal.mk

ifeq ($(STM32F4XX),1)
include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
endif

include ./board/board.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk

ifeq ($(STM32F4XX),1)
include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.mk
endif

# Other files (optional).
include $(CHIBIOS)/os/hal/lib/streams/streams.mk
include $(CHIBIOS)/os/various/fatfs_bindings/fatfs.mk
include common/common.mk
include hydrabus/hydrabus.mk
include drv/stm32cube/stm32cube.mk

ifeq ($(STM32F4XX),1)
include drv/stm32cube/stm32f4xx_hal.mk
endif

ifeq ($(HYDRAFW_NFC),1)
include hydranfc/trf7970a/trf7970a.mk
include hydranfc/hydranfc.mk
endif

# Define linker script file here
ifeq ($(STM32F4XX),1)
LDSCRIPT= $(STARTUPLD)/STM32F405xG.ld
endif

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(STARTUPSRC) \
       $(KERNSRC) \
       $(PORTSRC) \
       $(OSALSRC) \
       $(HALSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC) \
       $(FATFSSRC) \
       $(STREAMSSRC) \
       $(CHIBIOS)/os/various/syscalls.c \
       $(COMMONSRC) \
       $(HYDRABUSSRC) \
       $(STM32CUBESRC) \
       tokenline/tokenline.c \
       main.c

ifeq ($(STM32F4XX),1)
CSRC += $(STM32F4XX_HAL_SRC)
endif

ifeq ($(HYDRAFW_NFC),1)
CSRC += $(TRFSRC) \
        $(HYDRANFCSRC)
endif

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(STM32CUBESRC_ASM)
ASMXSRC = $(STARTUPASM) $(PORTASM) $(OSALASM)

INCDIR = $(CHIBIOS)/os/license \
         $(STARTUPINC) $(KERNINC) $(PORTINC) $(OSALINC) \
         $(HALINC) $(PLATFORMINC) $(BOARDINC) \
         $(STREAMSINC) $(LWINC) $(FATFSINC) \
         $(CHIBIOS)/os/various \
         $(COMMONINC) \
         $(HYDRABUSINC) \
         $(STM32CUBEINC) \
         tokenline

ifeq ($(STM32F4XX),1)
INCDIR += $(STM32F4XX_HAL_INC)
endif

ifeq ($(HYDRAFW_NFC),1)
INCDIR += $(TRFINC) \
          $(HYDRANFCINC)
endif

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

ifeq ($(STM32F4XX),1)
MCU  = cortex-m4
endif

#TRGT = arm-elf-
TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra -Wundef

#
# Compiler settings
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = $(HYDRAFW_OPTS)

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

# Flash tool with arguments depending on platform
ifeq ($(OS), Windows_NT)
FLASH_CMD = ../utils/windows_dfu_util/DfuSeCommand.exe -c --de 0 -d --fn
else
FLASH_CMD = dfu-util -d 0483:df11 -a 0 -R -D
endif

#
# End of user defines
##############################################################################

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC
include $(RULESPATH)/rules.mk

# Anything that depends on FORCE will be considered out-of-date
FORCE:
.PHONY: FORCE

%.hdr: FORCE
	$(OUT_LOG) echo Creating ./common/hydrafw_version.hdr
	$(OUT_CMD) rm -f $(OBJDIR)/common.o
	$(OUT_CMD) build-scripts/hydrafw-version ./common/hydrafw_version.hdr

%.dfu: %.hex $(LDSCRIPT)
	$(OUT_LOG) echo Creating $@
	$(OUT_CMD) $(PYTHON) build-scripts/dfu-convert.py -r $(FW_REVISION) -i $< $@

# This rule hook is defined in the ChibiOS build system
PRE_MAKE_ALL_RULE_HOOK: ./common/hydrafw_version.hdr

# This rule hook is defined in the ChibiOS build system
POST_MAKE_ALL_RULE_HOOK: $(BUILDDIR)/$(PROJECT).dfu

# This rule hook for `make clean` is defined in the ChibiOS build system
CLEAN_RULE_HOOK: $(shell rm -f ./common/hydrafw_version.hdr)

# Custom rule to flash firmware when hydrabus is connected in dfu mode
flash: $(BUILDDIR)/$(PROJECT).dfu
	$(OUT_LOG) echo Flashing $^
	$(OUT_CMD) $(FLASH_CMD) $^
