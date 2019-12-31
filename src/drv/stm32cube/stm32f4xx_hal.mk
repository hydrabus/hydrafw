# List of all the STM32F4XX_HAL files (required by stm32cube.mk)
STM32F4XX_HAL_SRC = ./drv/stm32cube/stm32f4xx_hal/stm32f4xx_hal_msp.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_gpio.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_spi.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_uart.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_adc.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_dac.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_dac_ex.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_tim.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_tim_ex.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_can.c \
                    ./drv/stm32cube/stm32f4xx_hal/src/stm32f4xx_hal_smartcard.c

# Required include directories
STM32F4XX_HAL_INC = ./drv/stm32cube \
                    ./drv/stm32cube/stm32f4xx_hal \
                    ./drv/stm32cube/stm32f4xx_hal/inc \
                    ./drv/stm32cube/stm32f4xx_hal/inc/Legacy
