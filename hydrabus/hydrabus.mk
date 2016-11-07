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
            hydrabus/hydrabus_mode_i2c.c \
            hydrabus/hydrabus_sump.c \
            hydrabus/hydrabus_mode_jtag.c \
            hydrabus/hydrabus_rng.c \
            hydrabus/hydrabus_mode_onewire.c \
            hydrabus/hydrabus_mode_twowire.c \
            hydrabus/hydrabus_mode_threewire.c \
            hydrabus/hydrabus_mode_can.c \
            hydrabus/hydrabus_bbio.c \
            hydrabus/hydrabus_bbio_spi.c \
            hydrabus/hydrabus_bbio_pin.c \
            hydrabus/hydrabus_bbio_can.c \
            hydrabus/hydrabus_bbio_uart.c \
            hydrabus/hydrabus_bbio_i2c.c \
            hydrabus/hydrabus_bbio_rawwire.c \
            hydrabus/hydrabus_freq.c \
            hydrabus/hydrabus_bbio_onewire.c

# Required include directories
HYDRABUSINC = ./hydrabus
