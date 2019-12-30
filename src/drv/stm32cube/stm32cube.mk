# List of all the stm32cube used files
STM32CUBESRC = ./drv/stm32cube/stm32f4xx_hal_msp.c \
              ./drv/stm32cube/src/stm32f4xx_hal_gpio.c \
              ./drv/stm32cube/src/stm32f4xx_hal_spi.c \
              ./drv/stm32cube/src/stm32f4xx_hal_uart.c \
              ./drv/stm32cube/src/stm32f4xx_hal_adc.c \
              ./drv/stm32cube/src/stm32f4xx_hal_dac.c \
              ./drv/stm32cube/src/stm32f4xx_hal_dac_ex.c \
              ./drv/stm32cube/src/stm32f4xx_hal_tim.c \
              ./drv/stm32cube/src/stm32f4xx_hal_tim_ex.c \
              ./drv/stm32cube/src/stm32f4xx_hal_can.c \
              ./drv/stm32cube/src/stm32f4xx_hal_smartcard.c \
              ./drv/stm32cube/bsp.c \
              ./drv/stm32cube/bsp_adc.c \
              ./drv/stm32cube/bsp_dac.c \
              ./drv/stm32cube/bsp_pwm.c \
              ./drv/stm32cube/bsp_gpio.c \
              ./drv/stm32cube/bsp_i2c.c \
              ./drv/stm32cube/bsp_spi.c \
              ./drv/stm32cube/bsp_uart.c \
              ./drv/stm32cube/bsp_smartcard.c \
              ./drv/stm32cube/bsp_rng.c \
              ./drv/stm32cube/bsp_can.c \
              ./drv/stm32cube/bsp_freq.c \
              ./drv/stm32cube/bsp_trigger.c \
              ./drv/stm32cube/bsp_tim.c

# Required include directories
STM32CUBEINC = ./drv/stm32cube \
              ./drv/stm32cube/inc \
              ./drv/stm32cube/inc/Legacy
