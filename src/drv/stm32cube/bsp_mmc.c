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
#include "bsp_mmc.h"
#include "bsp_mmc_conf.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define MMC_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_MMC (1)

static MMC_HandleTypeDef mmc_handle[NB_MMC];
static mode_config_proto_t* mmc_mode_conf[NB_MMC];
static volatile uint16_t dummy_read;

/**
  * @brief  Init low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: MMC dev num
  * @retval None
  */
/*
  This function replaces HAL_MMC_MspInit() in order to manage multiple devices.
  HAL_MMC_MspInit() shall be empty/not defined
*/
static void mmc_gpio_hw_init(bsp_dev_mmc_t dev_num)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	(void)dev_num;

	/* Enable the MMC peripheral */
	__SDIO_CLK_ENABLE();

	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Pull  = GPIO_NOPULL;
	GPIO_InitStructure.Speed = BSP_MMC_GPIO_SPEED;
	GPIO_InitStructure.Alternate = BSP_MMC_AF;


	GPIO_InitStructure.Pin = BSP_MMC_CK_PIN;
	HAL_GPIO_Init(BSP_MMC_CK_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;

	GPIO_InitStructure.Pin = BSP_MMC_CMD_PIN;
	HAL_GPIO_Init(BSP_MMC_CMD_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = BSP_MMC_D0_PIN;
	HAL_GPIO_Init(BSP_MMC_D0_PORT, &GPIO_InitStructure);
}

/**
  * @brief  DeInit low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: MMC dev num
  * @retval None
  */
/*
  This function replaces HAL_MMC_MspDeInit() in order to manage multiple devices.
  HAL_MMC_MspDeInit() shall be empty/not defined
*/
static void mmc_gpio_hw_deinit(bsp_dev_mmc_t dev_num)
{
	(void)dev_num;
	__SDIO_FORCE_RESET();
	__SDIO_RELEASE_RESET();
	/* Disable peripherals GPIO */
	HAL_GPIO_DeInit(BSP_MMC_CK_PORT, BSP_MMC_CK_PIN);
	HAL_GPIO_DeInit(BSP_MMC_CMD_PORT, BSP_MMC_CMD_PIN);
	HAL_GPIO_DeInit(BSP_MMC_D0_PORT, BSP_MMC_D0_PIN);
}

/**
  * @brief  MMCx error treatment function.
  * @param  dev_num: MMC dev num
  * @retval None
  */
static void mmc_error(bsp_dev_mmc_t dev_num)__attribute__((unused));
static void mmc_error(bsp_dev_mmc_t dev_num)
{
	(void)dev_num;
	if(bsp_mmc_deinit(dev_num) == BSP_OK) {
		/* Re-Initialize the MMC comunication bus */
		bsp_mmc_init(dev_num, mmc_mode_conf[dev_num]);
	}
}

/**
  * @brief  Init MMC device.
  * @param  dev_num: MMC dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_mmc_init(bsp_dev_mmc_t dev_num, mode_config_proto_t* mode_conf)
{
	MMC_HandleTypeDef* hmmc;
	bsp_status_t status;

	mmc_mode_conf[dev_num] = mode_conf;
	hmmc = &mmc_handle[dev_num];

	mmc_gpio_hw_init(dev_num);

	__HAL_MMC_RESET_HANDLE_STATE(hmmc);

	hmmc->Instance = BSP_MMC;
	hmmc->Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
	hmmc->Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
	hmmc->Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
	hmmc->Init.BusWide = SDIO_BUS_WIDE_1B;
	hmmc->Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
	hmmc->Init.ClockDiv = SDIO_INIT_CLK_DIV;

	status = (bsp_status_t) HAL_MMC_Init(hmmc);

	if(status != BSP_OK) {
		return status;
	}

	status = (bsp_status_t) HAL_MMC_InitCard(hmmc);

	return status;
}

/**
  * @brief  De-initialize the MMC comunication bus
  * @param  dev_num: MMC dev num.
  * @retval status: status of the deinit.
  */
bsp_status_t bsp_mmc_deinit(bsp_dev_mmc_t dev_num)
{
	MMC_HandleTypeDef* hmmc;
	bsp_status_t status;

	hmmc = &mmc_handle[dev_num];

	/* De-initialize the MMC comunication bus */
	status = (bsp_status_t) HAL_MMC_DeInit(hmmc);

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	mmc_gpio_hw_deinit(dev_num);

	return status;
}

uint32_t *bsp_mmc_get_cid(bsp_dev_mmc_t dev_num)
{
	MMC_HandleTypeDef* hmmc;

	hmmc = &mmc_handle[dev_num];

	return hmmc->CID;
}

uint32_t *bsp_mmc_get_csd(bsp_dev_mmc_t dev_num)
{
	MMC_HandleTypeDef* hmmc;

	hmmc = &mmc_handle[dev_num];

	return hmmc->CSD;
}

bsp_status_t bsp_mmc_change_bus_width(bsp_dev_mmc_t dev_num, uint8_t bus_size)
{
	MMC_HandleTypeDef* hmmc;
	bsp_status_t status;

	hmmc = &mmc_handle[dev_num];

	switch(bus_size) {
	case 1:
		status = (bsp_status_t) HAL_MMC_ConfigWideBusOperation(hmmc, SDIO_BUS_WIDE_1B);
		break;
	case 4:
		status = (bsp_status_t) HAL_MMC_ConfigWideBusOperation(hmmc, SDIO_BUS_WIDE_4B);
		break;
	/* Unused, pins are not present on Hydrabus v1
	case 8:
		status = (bsp_status_t) HAL_MMC_ConfigWideBusOperation(hmmc, SDIO_BUS_WIDE_8B);
		break;
	*/
	default:
		status = BSP_ERROR;
	}
	return status;
}

bsp_status_t bsp_mmc_read_extcsd(bsp_dev_mmc_t dev_num, uint8_t * extcsd)
{
	MMC_HandleTypeDef* hmmc;
	hmmc = &mmc_handle[dev_num];
	SDIO_CmdInitTypeDef cmd;
	SDIO_DataInitTypeDef config;

	bsp_status_t status;
	uint32_t tickstart, data, dataremaining=512;
	uint8_t count;

	cmd.CmdIndex = SDMMC_CMD_HS_SEND_EXT_CSD;
	cmd.Argument = 0;
	cmd.Response = SDIO_RESPONSE_SHORT;
	cmd.WaitForInterrupt = SDIO_WAIT_NO;
	cmd.CPSM = SDIO_CPSM_ENABLE;

	config.DataTimeOut   = SDMMC_DATATIMEOUT;
	config.DataLength    = 512;
	config.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
	config.TransferDir   = SDIO_TRANSFER_DIR_TO_SDIO;
	config.TransferMode  = SDIO_TRANSFER_MODE_BLOCK;
	config.DPSM          = SDIO_DPSM_ENABLE;
	(void)SDIO_ConfigData(hmmc->Instance, &config);

	status = (bsp_status_t)SDIO_SendCommand(hmmc->Instance, &cmd);
	if(status != BSP_OK) {
		return status;
	}

	tickstart = HAL_GetTick();
	while (!__HAL_MMC_GET_FLAG(hmmc, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND)) {
		if(__HAL_MMC_GET_FLAG(hmmc, SDIO_FLAG_RXFIFOHF) && (dataremaining > 0U)) {
			for(count = 0U; count < 8U; count++) {
				data = SDIO_ReadFIFO(hmmc->Instance);
				*extcsd = (uint8_t)(data & 0xFFU);
				extcsd++;
				dataremaining--;
				*extcsd = (uint8_t)((data >> 8U) & 0xFFU);
				extcsd++;
				dataremaining--;
				*extcsd = (uint8_t)((data >> 16U) & 0xFFU);
				extcsd++;
				dataremaining--;
				*extcsd = (uint8_t)((data >> 24U) & 0xFFU);
				extcsd++;
				dataremaining--;
			}
		}

		if (((HAL_GetTick() - tickstart) >= MMC_TIMEOUT_MAX)) {
			__HAL_MMC_CLEAR_FLAG(hmmc, SDMMC_STATIC_FLAGS);
			return BSP_TIMEOUT;
		}
	}
	__HAL_MMC_CLEAR_FLAG(hmmc, SDMMC_STATIC_FLAGS);
	return BSP_OK;
}

bsp_status_t bsp_mmc_get_info(bsp_dev_mmc_t dev_num, bsp_mmc_info_t * mmc_info)
{
	MMC_HandleTypeDef* hmmc;

	hmmc = &mmc_handle[dev_num];

	HAL_MMC_GetCardInfo(hmmc, (HAL_MMC_CardInfoTypeDef *)mmc_info);

	return BSP_OK;
}

/**
  * @brief  Sends a Byte in blocking mode and return the status.
  * @param  dev_num: MMC dev num.
  * @param  tx_data: data to send.
  * @param  nb_data: Number of data to send.
  * @retval status of the transfer.
  */
bsp_status_t bsp_mmc_write_block(bsp_dev_mmc_t dev_num, uint8_t* tx_data, uint32_t block_number)
{
	MMC_HandleTypeDef* hmmc;
	bsp_status_t status;

	hmmc = &mmc_handle[dev_num];

	status = (bsp_status_t) HAL_MMC_WriteBlocks(hmmc, tx_data, block_number, 1, MMC_TIMEOUT_MAX);

	return status;
}

/**
  * @brief  Read a Byte in blocking mode and return the status.
  * @param  dev_num: MMC dev num.
  * @param  rx_data: Data to receive.
  * @param  nb_data: Number of data to receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_mmc_read_block(bsp_dev_mmc_t dev_num, uint8_t* rx_data, uint32_t block_number)
{
	MMC_HandleTypeDef* hmmc;
	bsp_status_t status;

	hmmc = &mmc_handle[dev_num];

	status = (bsp_status_t) HAL_MMC_ReadBlocks(hmmc, rx_data, block_number, 1, MMC_TIMEOUT_MAX);

	return status;
}
