# List of all the common related files.
COMMONSRC = common/common.c \
			common/exec.c \
            common/microsd.c \
            common/usb1cfg.c \
            common/usb2cfg.c \
            common/fault_handler.c \
            common/script.c

COMMONSRC_ASM = common/fault_handler_asm.s

# Required include directories
COMMONINC = ./common
