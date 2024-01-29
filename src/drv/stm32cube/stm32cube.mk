# List of all the stm32cube used files
STM32CUBESRC = ./drv/stm32cube/bsp.c \
               ./drv/stm32cube/bsp_adc.c \
               ./drv/stm32cube/bsp_pwm.c \
               ./drv/stm32cube/bsp_gpio.c \
               ./drv/stm32cube/bsp_i2c_master.c \
               ./drv/stm32cube/bsp_i2c_slave.c \
               ./drv/stm32cube/bsp_spi.c \
               ./drv/stm32cube/bsp_uart.c \
               ./drv/stm32cube/bsp_smartcard.c \
               ./drv/stm32cube/bsp_freq.c \
               ./drv/stm32cube/bsp_trigger.c \
               ./drv/stm32cube/bsp_tim.c \
               ./drv/stm32cube/bsp_sdio.c \
               ./drv/stm32cube/bsp_mmc.c \
               ./drv/stm32cube/bsp_sd.c \
               ./drv/stm32cube/bsp_fault_handler.c \
               ./drv/stm32cube/bsp_print_dbg.c

STM32CUBESRC_ASM = ./drv/stm32cube/bsp_fault_handler_asm.s

# Required include directories
STM32CUBEINC = ./drv/stm32cube
