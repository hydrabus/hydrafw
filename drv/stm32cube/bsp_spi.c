/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "bsp_spi.h"
#include "bsp_spi_conf.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define SPIx_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_SPI (BSP_DEV_SPI_END)
static SPI_HandleTypeDef spi_handle[NB_SPI];
static mode_config_proto_t* spi_mode_conf[NB_SPI];

/**
  * @brief  Init low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: SPI dev num
  * @retval None
  */
/*
  This function replaces HAL_SPI_MspInit() in order to manage multiple devices.
  HAL_SPI_MspInit() shall be empty/not defined
*/
static void spi_gpio_hw_init(bsp_dev_spi_t dev_num, uint32_t gpio_sck_miso_mosi_pull)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	if(dev_num == BSP_DEV_SPI1) {
		/* Enable the SPI peripheral */
		__SPI1_CLK_ENABLE();

		/* SPI NSS pin configuration */
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStructure.Pull  = GPIO_PULLUP;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStructure.Pin = BSP_SPI1_NSS_PIN;
		HAL_GPIO_Init(BSP_SPI1_NSS_PORT, &GPIO_InitStructure);

		/* SPI SCK pin configuration */
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull  = gpio_sck_miso_mosi_pull;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStructure.Alternate = BSP_SPI1_AF;
		GPIO_InitStructure.Pin = BSP_SPI1_SCK_PIN;
		HAL_GPIO_Init(BSP_SPI1_SCK_PORT, &GPIO_InitStructure);

		/* SPI MISO pin configuration */
		GPIO_InitStructure.Pin = BSP_SPI1_MISO_PIN;
		HAL_GPIO_Init(BSP_SPI1_MISO_PORT, &GPIO_InitStructure);

		/* SPI MOSI pin configuration */
		GPIO_InitStructure.Pin = BSP_SPI1_MOSI_PIN;
		HAL_GPIO_Init(BSP_SPI1_MOSI_PORT, &GPIO_InitStructure);

	} else {
		/* Enable the SPI peripheral */
		__SPI2_CLK_ENABLE();

		/* SPI NSS pin configuration */
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStructure.Pull  = GPIO_PULLUP;
		GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
		GPIO_InitStructure.Pin = BSP_SPI2_NSS_PIN;
		HAL_GPIO_Init(BSP_SPI2_NSS_PORT, &GPIO_InitStructure);

		/* SPI SCK pin configuration */
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull  = gpio_sck_miso_mosi_pull;
		GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
		GPIO_InitStructure.Alternate = BSP_SPI2_AF;
		GPIO_InitStructure.Pin = BSP_SPI2_SCK_PIN;
		HAL_GPIO_Init(BSP_SPI2_SCK_PORT, &GPIO_InitStructure);

		/* SPI MISO pin configuration */
		GPIO_InitStructure.Pin = BSP_SPI2_MISO_PIN;
		HAL_GPIO_Init(BSP_SPI2_MISO_PORT, &GPIO_InitStructure);

		/* SPI MOSI pin configuration */
		GPIO_InitStructure.Pin = BSP_SPI2_MOSI_PIN;
		HAL_GPIO_Init(BSP_SPI2_MOSI_PORT, &GPIO_InitStructure);
	}

}

/**
  * @brief  DeInit low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: SPI dev num
  * @retval None
  */
/*
  This function replaces HAL_SPI_MspDeInit() in order to manage multiple devices.
  HAL_SPI_MspDeInit() shall be empty/not defined
*/
static void spi_gpio_hw_deinit(bsp_dev_spi_t dev_num)
{
	if(dev_num == BSP_DEV_SPI1) {
		/* Reset peripherals */
		__SPI1_FORCE_RESET();
		__SPI1_RELEASE_RESET();
		__SPI1_CLK_DISABLE();

		/* Disable peripherals and GPIO Clocks */
		HAL_GPIO_DeInit(BSP_SPI1_NSS_PORT, BSP_SPI1_NSS_PIN);
		HAL_GPIO_DeInit(BSP_SPI1_SCK_PORT, BSP_SPI1_SCK_PIN);
		HAL_GPIO_DeInit(BSP_SPI1_MISO_PORT, BSP_SPI1_MISO_PIN);
		HAL_GPIO_DeInit(BSP_SPI1_MOSI_PORT, BSP_SPI1_MOSI_PIN);
	} else {
		/* Reset peripherals */
		__SPI2_FORCE_RESET();
		__SPI2_RELEASE_RESET();
		__SPI2_CLK_DISABLE();

		/* Disable peripherals and GPIO Clocks */
		HAL_GPIO_DeInit(BSP_SPI2_NSS_PORT, BSP_SPI2_NSS_PIN);
		HAL_GPIO_DeInit(BSP_SPI2_SCK_PORT, BSP_SPI2_SCK_PIN);
		HAL_GPIO_DeInit(BSP_SPI2_MISO_PORT, BSP_SPI2_MISO_PIN);
		HAL_GPIO_DeInit(BSP_SPI2_MOSI_PORT, BSP_SPI2_MOSI_PIN);
	}
}

/**
  * @brief  SPIx error treatment function.
  * @param  dev_num: SPI dev num
  * @retval None
  */
static void spi_error(bsp_dev_spi_t dev_num)
{
	if(bsp_spi_deinit(dev_num) == BSP_OK) {
		/* Re-Initialize the SPI comunication bus */
		bsp_spi_init(dev_num, spi_mode_conf[dev_num]);
	}
}

/**
  * @brief  Init SPI device.
  * @param  dev_num: SPI dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_spi_init(bsp_dev_spi_t dev_num, mode_config_proto_t* mode_conf)
{
	SPI_HandleTypeDef* hspi;
	bsp_status_t status;
	uint32_t cpol;
	uint32_t cpha;
	uint32_t gpio_sck_miso_mosi_pull;

	spi_mode_conf[dev_num] = mode_conf;
	hspi = &spi_handle[dev_num];

	switch(mode_conf->dev_gpio_pull) {
	case MODE_CONFIG_DEV_GPIO_PULLUP:
		gpio_sck_miso_mosi_pull = GPIO_PULLUP;
		break;

	case MODE_CONFIG_DEV_GPIO_PULLDOWN:
		gpio_sck_miso_mosi_pull = GPIO_PULLDOWN;
		break;

	default:
	case MODE_CONFIG_DEV_GPIO_NOPULL:
		gpio_sck_miso_mosi_pull = GPIO_NOPULL;
		break;
	}

	spi_gpio_hw_init(dev_num, gpio_sck_miso_mosi_pull);

	__HAL_SPI_RESET_HANDLE_STATE(hspi);

	if(dev_num == BSP_DEV_SPI1) {
		hspi->Instance = BSP_SPI1;
	} else { /* SPI2 */
		hspi->Instance = BSP_SPI2;
	}
	/* SW NSS user shall call bsp_spi_select()/bsp_spi_unselect() */
	hspi->Init.NSS = SPI_NSS_SOFT;
	hspi->Init.BaudRatePrescaler = ((7 - mode_conf->dev_speed) * SPI_CR1_BR_0);
	hspi->Init.Direction = SPI_DIRECTION_2LINES;

	if(mode_conf->dev_phase == 0)
		cpha = SPI_PHASE_1EDGE;
	else
		cpha = SPI_PHASE_2EDGE;

	if(mode_conf->dev_polarity == 0)
		cpol = SPI_POLARITY_LOW;
	else
		cpol = SPI_POLARITY_HIGH;

	hspi->Init.CLKPhase = cpha;
	hspi->Init.CLKPolarity = cpol;
	hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi->Init.CRCPolynomial = 0;
	hspi->Init.DataSize = SPI_DATASIZE_8BIT;

	if(mode_conf->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_MSB)
		hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
	else
		hspi->Init.FirstBit = SPI_FIRSTBIT_LSB;

	hspi->Init.TIMode = SPI_TIMODE_DISABLED;

	if(mode_conf->dev_mode == DEV_SPI_SLAVE)
		hspi->Init.Mode = SPI_MODE_SLAVE;
	else
		hspi->Init.Mode = SPI_MODE_MASTER;

	status = HAL_SPI_Init(hspi);

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(hspi);

	return status;
}

/**
  * @brief  De-initialize the SPI comunication bus
  * @param  dev_num: SPI dev num.
  * @retval status: status of the deinit.
  */
bsp_status_t bsp_spi_deinit(bsp_dev_spi_t dev_num)
{
	SPI_HandleTypeDef* hspi;
	bsp_status_t status;

	hspi = &spi_handle[dev_num];

	/* De-initialize the SPI comunication bus */
	status = HAL_SPI_DeInit(hspi);

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	spi_gpio_hw_deinit(dev_num);

	return status;
}

/**
  * @brief  Enable Chip Select pin (active low).
  * @param  dev_num: SPI dev num.
  */
void bsp_spi_select(bsp_dev_spi_t dev_num)
{
	if(dev_num == BSP_DEV_SPI1) {
		HAL_GPIO_WritePin(BSP_SPI1_NSS_PORT, BSP_SPI1_NSS_PIN, GPIO_PIN_RESET);
	} else { /* SPI2 */
		HAL_GPIO_WritePin(BSP_SPI2_NSS_PORT, BSP_SPI2_NSS_PIN, GPIO_PIN_RESET);
	}
}

/**
  * @brief  Disable Chip Select pin (inactive high).
  * @param  dev_num: SPI dev num.
  */
void bsp_spi_unselect(bsp_dev_spi_t dev_num)
{
	if(dev_num == BSP_DEV_SPI1) {
		HAL_GPIO_WritePin(BSP_SPI1_NSS_PORT, BSP_SPI1_NSS_PIN, GPIO_PIN_SET);
	} else { /* SPI2 */
		HAL_GPIO_WritePin(BSP_SPI2_NSS_PORT, BSP_SPI2_NSS_PIN, GPIO_PIN_SET);
	}
}

/**
  * @brief  Get Chip Select pin state
  * @param  dev_num: SPI dev num.
  * @retval Status of the pin.
  */
uint8_t bsp_spi_get_cs(bsp_dev_spi_t dev_num)
{
	if(dev_num == BSP_DEV_SPI1) {
		return HAL_GPIO_ReadPin(BSP_SPI1_NSS_PORT, BSP_SPI1_NSS_PIN);
	} else { /* SPI2 */
		return HAL_GPIO_ReadPin(BSP_SPI2_NSS_PORT, BSP_SPI2_NSS_PIN);
	}
}

/**
  * @brief  Checks if the SPI receive buffer is empty
  * @param  dev_num: SPI dev num.
  * @retval 0 if empty, 1 if data
  */
uint8_t bsp_spi_rxne(bsp_dev_spi_t dev_num)
{
	SPI_HandleTypeDef* hspi;
	hspi = &spi_handle[dev_num];

	return __HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE);
}

/**
  * @brief  Sends a Byte in blocking mode and return the status.
  * @param  dev_num: SPI dev num.
  * @param  tx_data: data to send.
  * @param  nb_data: Number of data to send.
  * @retval status of the transfer.
  */
bsp_status_t bsp_spi_write_u8(bsp_dev_spi_t dev_num, uint8_t* tx_data, uint8_t nb_data)
{
	SPI_HandleTypeDef* hspi;
	hspi = &spi_handle[dev_num];

	bsp_status_t status;
	status = HAL_SPI_Transmit(hspi, tx_data, nb_data, SPIx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		spi_error(dev_num);
	}
	return status;
}

/**
  * @brief  Read a Byte in blocking mode and return the status.
  * @param  dev_num: SPI dev num.
  * @param  rx_data: Data to receive.
  * @param  nb_data: Number of data to receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_spi_read_u8(bsp_dev_spi_t dev_num, uint8_t* rx_data, uint8_t nb_data)
{
	SPI_HandleTypeDef* hspi;
	hspi = &spi_handle[dev_num];

	bsp_status_t status;
	status = HAL_SPI_Receive(hspi, rx_data, nb_data, SPIx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		spi_error(dev_num);
	}
	return status;
}

/**
  * @brief  Send a byte then Read a byte through the SPI interface.
  * @param  tx_data: Data to send.
  * @param  rx_data: Data to receive.
  * @param  nb_data: Number of data to send & receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_spi_write_read_u8(bsp_dev_spi_t dev_num, uint8_t* tx_data, uint8_t* rx_data, uint8_t nb_data)
{
	SPI_HandleTypeDef* hspi;
	hspi = &spi_handle[dev_num];

	bsp_status_t status;
	status = HAL_SPI_TransmitReceive(hspi, tx_data, rx_data, nb_data, SPIx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		spi_error(dev_num);
	}
	return status;
}

