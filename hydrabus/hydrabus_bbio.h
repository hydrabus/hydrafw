/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2015 Nicolas OBERLI
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Basic BBIO modes
 */
#define BBIO_RESET	0b00000000
#define BBIO_SPI	0b00000001
#define BBIO_I2C	0b00000010
#define BBIO_UART	0b00000011
#define BBIO_1WIRE	0b00000100
#define BBIO_RAWWIRE	0b00000101
#define BBIO_JTAG	0b00000110

#define BBIO_RESET_HW	0b00001111
#define BBIO_PWM	0b00010010
#define BBIO_PWM_CLEAR	0b00010010
#define BBIO_VOLT	0b00010100
#define BBIO_VOLT_CONT	0b00010101
#define BBIO_FREQ	0b00010110

/*
 * SPI-specific commands
 */
#define BBIO_SPI_CS_LOW		0b00000010
#define BBIO_SPI_CS_HIGH	0b00000011
#define BBIO_SPI_WRITE_READ	0b00000100
#define BBIO_SPI_WRITE_READ_NCS	0b00000101
#define BBIO_SPI_SNIFF_ALL	0b00001101
#define BBIO_SPI_SNIFF_CS_LOW	0b00001110
#define BBIO_SPI_SNIFF_CS_HIGH	0b00001111
#define BBIO_SPI_SNIFF_CS_HIGH	0b00001111
#define BBIO_SPI_BULK_TRANSFER	0b00010000
#define BBIO_SPI_CONFIG_PERIPH	0b01000000
#define BBIO_SPI_SET_SPEED	0b01100000
#define BBIO_SPI_CONFIG		0b10000000

int cmd_bbio(t_hydra_console *con);
