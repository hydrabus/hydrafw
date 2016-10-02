/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX

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
#include "bsp_uart.h"
#include "bsp_uart_conf.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define UARTx_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_UART (BSP_DEV_UART_END)
#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

#define CLOCK_DIV8 (8)
#define CLOCK_DIV16 (16)

static UART_HandleTypeDef uart_handle[NB_UART];
static mode_config_proto_t* uart_mode_conf[NB_UART];
volatile uint16_t dummy_read;

/**
  * @brief  Init low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: UART dev num
  * @retval None
  */
/*
  This function replaces HAL_UART_MspInit() in order to manage multiple devices.
  HAL_UART_MspInit() shall be empty/not defined
*/
static void uart_gpio_hw_init(bsp_dev_uart_t dev_num)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if(dev_num == BSP_DEV_UART1) {
		/* Enable the UART peripheral */
		__USART1_CLK_ENABLE();

		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull  = GPIO_NOPULL;
		GPIO_InitStructure.Speed = BSP_UART1_GPIO_SPEED;

		/* UART1 TX pin configuration */
		GPIO_InitStructure.Alternate = BSP_UART1_AF;
		GPIO_InitStructure.Pin = BSP_UART1_TX_PIN;
		HAL_GPIO_Init(BSP_UART1_TX_PORT, &GPIO_InitStructure);

		/* UART1 RX pin configuration */
		GPIO_InitStructure.Pin = BSP_UART1_RX_PIN;
		HAL_GPIO_Init(BSP_UART1_RX_PORT, &GPIO_InitStructure);

	} else {
		/* Enable the UART peripheral */
		__USART2_CLK_ENABLE();

		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull  = GPIO_NOPULL;
		GPIO_InitStructure.Speed = BSP_UART2_GPIO_SPEED;

		/* UART2 TX pin configuration */
		GPIO_InitStructure.Alternate = BSP_UART2_AF;
		GPIO_InitStructure.Pin = BSP_UART2_TX_PIN;
		HAL_GPIO_Init(BSP_UART2_TX_PORT, &GPIO_InitStructure);

		/* UART2 RX pin configuration */
		GPIO_InitStructure.Pin = BSP_UART2_RX_PIN;
		HAL_GPIO_Init(BSP_UART2_RX_PORT, &GPIO_InitStructure);
	}

}

/**
  * @brief  DeInit low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: UART dev num
  * @retval None
  */
/*
  This function replaces HAL_UART_MspDeInit() in order to manage multiple devices.
  HAL_UART_MspDeInit() shall be empty/not defined
*/
static void uart_gpio_hw_deinit(bsp_dev_uart_t dev_num)
{
	if(dev_num == BSP_DEV_UART1) {
		/* Reset peripherals */
		__USART1_FORCE_RESET();
		__USART1_RELEASE_RESET();

		/* Disable peripherals GPIO */
		HAL_GPIO_DeInit(BSP_UART1_TX_PORT, BSP_UART1_TX_PIN);
		HAL_GPIO_DeInit(BSP_UART1_RX_PORT, BSP_UART1_RX_PIN);
	} else {
		/* Reset peripherals */
		__USART2_FORCE_RESET();
		__USART2_RELEASE_RESET();

		/* Disable peripherals GPIO */
		HAL_GPIO_DeInit(BSP_UART2_TX_PORT, BSP_UART2_TX_PIN);
		HAL_GPIO_DeInit(BSP_UART2_RX_PORT, BSP_UART2_RX_PIN);
	}
}

/**
  * @brief  UARTx error treatment function.
  * @param  dev_num: UART dev num
  * @retval None
  */
static void uart_error(bsp_dev_uart_t dev_num)
{
	if(bsp_uart_deinit(dev_num) == BSP_OK) {
		/* Re-Initialize the UART comunication bus */
		bsp_uart_init(dev_num, uart_mode_conf[dev_num]);
	}
}

/**
  * @brief  Init UART device.
  * @param  dev_num: UART dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_uart_init(bsp_dev_uart_t dev_num, mode_config_proto_t* mode_conf)
{
	UART_HandleTypeDef* huart;
	bsp_status_t status;

	uart_mode_conf[dev_num] = mode_conf;
	huart = &uart_handle[dev_num];

	uart_gpio_hw_init(dev_num);

	__HAL_UART_RESET_HANDLE_STATE(huart);

	if(dev_num == BSP_DEV_UART1) {
		huart->Instance = BSP_UART1;
	} else { /* UART2 */
		huart->Instance = BSP_UART2;
	}
	huart->Init.BaudRate = mode_conf->dev_speed;

	if(huart->Init.BaudRate < 4800)
		huart->Init.OverSampling = UART_OVERSAMPLING_16;
	else
		huart->Init.OverSampling = UART_OVERSAMPLING_8;

	/* Check baudrate is not too low */
	if(huart->Init.BaudRate < 81)
		return BSP_ERROR;

	switch(mode_conf->dev_parity) {
	case 1: /* 8/even */
		huart->Init.Parity = UART_PARITY_EVEN;
		huart->Init.WordLength = UART_WORDLENGTH_9B;
		break;

	case 2: /* 8/odd */
		huart->Init.Parity = UART_PARITY_ODD;
		huart->Init.WordLength = UART_WORDLENGTH_9B;
		break;

	case 0: /* 8/none */
	default:
		huart->Init.Parity = UART_PARITY_NONE;
		huart->Init.WordLength = UART_WORDLENGTH_8B;
		break;
	}

	if(mode_conf->dev_stop_bit == 1)
		huart->Init.StopBits   = UART_STOPBITS_1;
	else
		huart->Init.StopBits   = UART_STOPBITS_2;

	huart->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	huart->Init.Mode       = UART_MODE_TX_RX;

	status = HAL_UART_Init(huart);

	/* Dummy read to flush old character */
	dummy_read = huart->Instance->DR;

	return status;
}

/**
  * @brief  De-initialize the UART comunication bus
  * @param  dev_num: UART dev num.
  * @retval status: status of the deinit.
  */
bsp_status_t bsp_uart_deinit(bsp_dev_uart_t dev_num)
{
	UART_HandleTypeDef* huart;
	bsp_status_t status;

	huart = &uart_handle[dev_num];

	/* De-initialize the UART comunication bus */
	status = HAL_UART_DeInit(huart);

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	uart_gpio_hw_deinit(dev_num);

	return status;
}

/**
  * @brief  Sends a Byte in blocking mode and return the status.
  * @param  dev_num: UART dev num.
  * @param  tx_data: data to send.
  * @param  nb_data: Number of data to send.
  * @retval status of the transfer.
  */
bsp_status_t bsp_uart_write_u8(bsp_dev_uart_t dev_num, uint8_t* tx_data, uint8_t nb_data)
{
	UART_HandleTypeDef* huart;
	huart = &uart_handle[dev_num];

	bsp_status_t status;
	status = HAL_UART_Transmit(huart, tx_data, nb_data, UARTx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		uart_error(dev_num);
	}
	return status;
}

/**
  * @brief  Read a Byte in blocking mode and return the status.
  * @param  dev_num: UART dev num.
  * @param  rx_data: Data to receive.
  * @param  nb_data: Number of data to receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_uart_read_u8(bsp_dev_uart_t dev_num, uint8_t* rx_data, uint8_t nb_data)
{
	UART_HandleTypeDef* huart;
	huart = &uart_handle[dev_num];

	bsp_status_t status;
	status = HAL_UART_Receive(huart, rx_data, nb_data, UARTx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		uart_error(dev_num);
	}
	return status;
}

/**
  * @brief  Send a byte then Read a byte through the UART interface.
  * @param  tx_data: Data to send.
  * @param  rx_data: Data to receive.
  * @param  nb_data: Number of data to send & receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_uart_write_read_u8(bsp_dev_uart_t dev_num, uint8_t* tx_data, uint8_t* rx_data, uint8_t nb_data)
{
	UART_HandleTypeDef* huart;
	huart = &uart_handle[dev_num];

	bsp_status_t status;
	status = HAL_UART_Transmit(huart, tx_data, nb_data, UARTx_TIMEOUT_MAX);
	if(status == BSP_OK) {
		status = HAL_UART_Receive(huart, rx_data, nb_data, UARTx_TIMEOUT_MAX);
	} else {
		uart_error(dev_num);
	}
	return status;
}

/**
  * @brief  Checks if the UART receive buffer is empty
  * @retval 0 if empty, 1 if data
  */
bsp_status_t bsp_uart_rxne(bsp_dev_uart_t dev_num)
{
	UART_HandleTypeDef* huart;
	huart = &uart_handle[dev_num];

	return __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
}

/** \brief Return final baud rate configured for over8=0 or over8=1.
 *
 * \param dev_num bsp_dev_uart_t
 * \return uint32_t final baudrate configured
 *
 */
uint32_t bsp_uart_get_final_baudrate(bsp_dev_uart_t dev_num)
{
	float f_baudrate;
	float f_baudrate_frac;
	uint32_t final_baudrate;
	uint32_t clock;
	uint32_t brr;
	UART_HandleTypeDef* huart;

	huart = &uart_handle[dev_num];
	brr = huart->Instance->BRR;

	if(huart->Init.OverSampling == UART_OVERSAMPLING_8) {

		if((huart->Instance == USART1) || (huart->Instance == USART6))
			clock = HAL_RCC_GetPCLK2Freq() / CLOCK_DIV8;
		else
			clock = HAL_RCC_GetPCLK1Freq() / CLOCK_DIV8;

		final_baudrate = brr >> 4;
		if(final_baudrate > 0) {
			f_baudrate_frac = (float)(brr & 0x07) / 8.0f;
			f_baudrate = ((float)final_baudrate) + f_baudrate_frac;
			final_baudrate = (uint32_t)((float)clock / f_baudrate);
		}
	} else {

		if((huart->Instance == USART1) || (huart->Instance == USART6))
			clock = HAL_RCC_GetPCLK2Freq() / CLOCK_DIV16;
		else
			clock = HAL_RCC_GetPCLK1Freq() / CLOCK_DIV16;

		final_baudrate = brr >> 4;
		if(final_baudrate > 0) {
			f_baudrate_frac = (float)(brr & 0x0F) / 16.0f;
			f_baudrate = ((float)final_baudrate) + f_baudrate_frac;
			final_baudrate = (uint32_t)((float)clock / f_baudrate);
		}
	}
	return final_baudrate;
}
