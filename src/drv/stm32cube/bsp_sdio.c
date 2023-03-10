/*
HydraBus/HydraNFC - Copyright (C) 2014-2023 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2014-2023 Nicolas OBERLI

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
#include "bsp_sdio.h"
#include "bsp_sdio_conf.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define SDIO_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_SDIO (1)

static mode_config_proto_t* sdio_mode_conf[NB_SDIO];
static volatile uint16_t dummy_read;

extern uint32_t reverse_u32(uint32_t value);

/**
  * @brief  Init low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: SDIO dev num
  * @retval None
  */
static void sdio_gpio_hw_init(bsp_dev_sdio_t dev_num)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	(void)dev_num;

	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Pull  = GPIO_NOPULL;
	GPIO_InitStructure.Speed = BSP_SDIO_GPIO_SPEED;
	GPIO_InitStructure.Alternate = BSP_SDIO_AF;


	GPIO_InitStructure.Pin = BSP_SDIO_CK_PIN;
	HAL_GPIO_Init(BSP_SDIO_CK_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;

	GPIO_InitStructure.Pin = BSP_SDIO_CMD_PIN;
	HAL_GPIO_Init(BSP_SDIO_CMD_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = BSP_SDIO_D0_PIN;
	HAL_GPIO_Init(BSP_SDIO_D0_PORT, &GPIO_InitStructure);
}

/**
  * @brief  DeInit low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: SDIO dev num
  * @retval None
  */
static void sdio_gpio_hw_deinit(bsp_dev_sdio_t dev_num)
{
	(void)dev_num;
	__SDIO_FORCE_RESET();
	__SDIO_RELEASE_RESET();
	__SDIO_DISABLE(SDIO);

	/* Disable peripherals GPIO */
	HAL_GPIO_DeInit(BSP_SDIO_CK_PORT, BSP_SDIO_CK_PIN);
	HAL_GPIO_DeInit(BSP_SDIO_CMD_PORT, BSP_SDIO_CMD_PIN);
	HAL_GPIO_DeInit(BSP_SDIO_D0_PORT, BSP_SDIO_D0_PIN);
}

/**
  * @brief  SDIOx error treatment function.
  * @param  dev_num: SDIO dev num
  * @retval None
  */
static void sdio_error(bsp_dev_sdio_t dev_num)__attribute__((unused));
static void sdio_error(bsp_dev_sdio_t dev_num)
{
	(void)dev_num;
	if(bsp_sdio_deinit(dev_num) == BSP_OK) {
		/* Re-Initialize the SDIO comunication bus */
		bsp_sdio_init(dev_num, sdio_mode_conf[dev_num]);
	}
}

/**
  * @brief  Init SDIO device.
  * @param  dev_num: SDIO dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_sdio_init(bsp_dev_sdio_t dev_num, mode_config_proto_t* mode_conf)
{
	SDIO_TypeDef* hsdio;
	SDIO_InitTypeDef init;
	bsp_status_t status;

	hsdio = SDIO;
	sdio_mode_conf[dev_num] = mode_conf;


	/* Enable the SDIO peripheral */
	__HAL_RCC_SDIO_CLK_ENABLE();
	__SDIO_ENABLE();
	__SDIO_FORCE_RESET();
	__SDIO_RELEASE_RESET();

	sdio_gpio_hw_init(dev_num);

	init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
	init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
	init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
	init.BusWide             = SDIO_BUS_WIDE_1B;
	init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
	init.ClockDiv            = SDIO_INIT_CLK_DIV;

	status = (bsp_status_t) SDIO_Init(hsdio, init);

	if(status != BSP_OK) {
		return status;
	}

	SDIO_PowerState_ON(hsdio);

	__SDIO_ENABLE(hsdio);

	return status;
}

/**
  * @brief  De-initialize the SDIO comunication bus
  * @param  dev_num: SDIO dev num.
  * @retval status: status of the deinit.
  */
bsp_status_t bsp_sdio_deinit(bsp_dev_sdio_t dev_num)
{
	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	sdio_gpio_hw_deinit(dev_num);

	return BSP_OK;
}

bsp_status_t bsp_sdio_get_instance(bsp_dev_sdio_t dev_num, void ** instance)
{
	(void) dev_num;
	*instance = SDIO;

	return BSP_OK;
}

bsp_status_t bsp_sdio_read_response(bsp_dev_sdio_t dev_num, uint32_t* response)
{
	SDIO_TypeDef* hsdio;
	(void) dev_num;

	hsdio = SDIO;

	response[0] = SDIO_GetResponse(hsdio, SDIO_RESP1);
	response[1] = SDIO_GetResponse(hsdio, SDIO_RESP2);
	response[2] = SDIO_GetResponse(hsdio, SDIO_RESP3);
	response[3] = SDIO_GetResponse(hsdio, SDIO_RESP4);
	return BSP_OK;
}

bsp_status_t bsp_sdio_send_command(bsp_dev_sdio_t dev_num, uint8_t cmdid, uint32_t argument, uint8_t resp_type)
{
	(void) dev_num;
	SDIO_CmdInitTypeDef  sdmmc_cmdinit;

	sdmmc_cmdinit.CmdIndex         = cmdid;
	sdmmc_cmdinit.Argument         = argument;
	switch(resp_type) {
	case 0:
		sdmmc_cmdinit.Response         = SDIO_RESPONSE_NO;
		break;
	case 1:
		sdmmc_cmdinit.Response         = SDIO_RESPONSE_SHORT;
		break;
	case 2:
		sdmmc_cmdinit.Response         = SDIO_RESPONSE_LONG;
	}
	sdmmc_cmdinit.WaitForInterrupt = SDIO_WAIT_NO;
	sdmmc_cmdinit.CPSM             = SDIO_CPSM_ENABLE;

	(void)SDIO_SendCommand(SDIO, &sdmmc_cmdinit);

	while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDSENT | SDIO_FLAG_CMDREND | SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT)){};

	if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CTIMEOUT)) {
		__SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_CMD_FLAGS);
		return BSP_TIMEOUT;
	}
	__SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_CMD_FLAGS);
	return BSP_OK;
}

bsp_status_t bsp_sdio_write_data(bsp_dev_sdio_t dev_num, uint8_t cmdid, uint32_t argument, uint8_t resp_type, uint8_t * data)
{
	(void) dev_num;
	SDIO_DataInitTypeDef config;
	uint32_t tmp;
	uint32_t response[4];
	uint32_t *pData = (uint32_t *)data;
	uint16_t count, remaining=512;

	/* Configure the SD DPSM (Data Path State Machine) */
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = BSP_SDIO_BLOCK_LEN;
	config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
	config.TransferDir   = SDIO_TRANSFER_DIR_TO_CARD;
	config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDIO_DPSM_ENABLE;
	(void)SDIO_ConfigData(SDIO, &config);

	bsp_sdio_send_command(dev_num, cmdid, argument, resp_type);
	bsp_sdio_read_response(dev_num, response);

	while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXUNDERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND)) {
		if(__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXFIFOHE)  && (remaining > 0)) {
			for(count = 0; count < 8; count++) {
				tmp = pData[(BSP_SDIO_BLOCK_LEN-remaining)>>2];
				tmp = reverse_u32(tmp);
				(void)SDIO_WriteFIFO(SDIO, &tmp);
				remaining -=4;
			}
		}
	}

	__SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_CMD_FLAGS);
	__SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_DATA_FLAGS);
	if(remaining == 0) {
		return BSP_OK;
	} else {
		return BSP_ERROR;
	}
}

bsp_status_t bsp_sdio_read_data(bsp_dev_sdio_t dev_num, uint8_t cmdid, uint32_t argument, uint8_t resp_type, uint8_t * data)
{
	(void) dev_num;
	SDIO_DataInitTypeDef config;
	uint32_t tmp;
	uint32_t *pData = (uint32_t *)data;
	uint16_t count, remaining = 512;

	/* Configure the SD DPSM (Data Path State Machine) */
	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = BSP_SDIO_BLOCK_LEN;
	config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
	config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
	config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDIO_DPSM_ENABLE;
	(void)SDIO_ConfigData(SDIO, &config);

	bsp_sdio_send_command(dev_num, cmdid, argument, resp_type);
	bsp_sdio_read_response(dev_num, (uint32_t *)data);


	while(!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND)) {
		if(__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXFIFOHF) && (remaining > 0)) {
			/* Read data from SDIO Rx FIFO */
			for(count = 0; count < 8; count++) {
				tmp = SDIO_ReadFIFO(SDIO);
				tmp = reverse_u32(tmp);
				pData[(BSP_SDIO_BLOCK_LEN-remaining)>>2] = tmp;
				remaining -=4;
			}
		}
	}
	__SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_CMD_FLAGS);
	__SDIO_CLEAR_FLAG(SDIO, SDIO_STATIC_DATA_FLAGS);
	if(remaining == 0) {
		return BSP_OK;
	} else {
		return BSP_ERROR;
	}
}
