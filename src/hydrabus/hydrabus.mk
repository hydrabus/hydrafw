# List of all the hydrabus related files.
HYDRABUSSRC = hydrabus/hydrabus.c \
            hydrabus/commands.c \
            hydrabus/hydrabus_adc.c \
            hydrabus/hydrabus_dac.c \
            hydrabus/hydrabus_pwm.c \
            hydrabus/gpio.c \
            hydrabus/hydrabus_mode.c \
            hydrabus/hydrabus_mode_spi.c \
            hydrabus/hydrabus_mode_uart.c \
            hydrabus/hydrabus_mode_smartcard.c \
            hydrabus/hydrabus_mode_i2c.c \
            hydrabus/hydrabus_sump.c \
            hydrabus/hydrabus_mode_jtag.c \
            hydrabus/hydrabus_rng.c \
            hydrabus/hydrabus_mode_onewire.c \
            hydrabus/hydrabus_mode_twowire.c \
            hydrabus/hydrabus_mode_threewire.c \
            hydrabus/hydrabus_mode_can.c \
            hydrabus/hydrabus_mode_flash.c \
            hydrabus/hydrabus_bbio.c \
            hydrabus/hydrabus_bbio_spi.c \
            hydrabus/hydrabus_bbio_pin.c \
            hydrabus/hydrabus_bbio_can.c \
            hydrabus/hydrabus_bbio_uart.c \
            hydrabus/hydrabus_bbio_smartcard.c \
            hydrabus/hydrabus_bbio_i2c.c \
            hydrabus/hydrabus_bbio_rawwire.c \
            hydrabus/hydrabus_freq.c \
            hydrabus/hydrabus_bbio_onewire.c \
            hydrabus/hydrabus_bbio_flash.c \
            hydrabus/hydrabus_bbio_adc.c \
            hydrabus/hydrabus_bbio_freq.c \
            hydrabus/hydrabus_sd.c \
            hydrabus/hydrabus_trigger.c \
            hydrabus/hydrabus_mode_wiegand.c \
            hydrabus/hydrabus_mode_lin.c \
            hydrabus/hydrabus_bbio_aux.c \
            hydrabus/hydrabus_aux.c \
            hydrabus/hydrabus_serprog.c \
            hydrabus/hydrabus_mode_mmc.c \
            hydrabus/hydrabus_bbio_mmc.c

# Required include directories
HYDRABUSINC = ./hydrabus
