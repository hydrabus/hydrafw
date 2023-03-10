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
#include "bsp_sd.h"
#include "bsp_sdio.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define SD_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_SD (1)

static SD_HandleTypeDef sd_handle[NB_SD];
static mode_config_proto_t* sd_mode_conf[NB_SD];
static volatile uint16_t dummy_read;

/**
  * @brief  Init SD device.
  * @param  dev_num: SD dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_sd_init(bsp_dev_sdio_t dev_num, mode_config_proto_t* mode_conf)
{
	SD_HandleTypeDef* hsd;
	bsp_status_t status;

	sd_mode_conf[dev_num] = mode_conf;
	hsd = &sd_handle[dev_num];

	hsd->ErrorCode = 0;
	hsd->Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
	hsd->Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
	hsd->Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
	hsd->Init.BusWide             = SDIO_BUS_WIDE_1B;
	hsd->Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
	hsd->Init.ClockDiv            = SDIO_INIT_CLK_DIV;

	status = (bsp_status_t) bsp_sdio_init(dev_num, mode_conf);

	bsp_sdio_get_instance(dev_num, (void *)&(hsd->Instance));

	if(status != BSP_OK) {
		return status;
	}

	status = (bsp_status_t) HAL_SD_InitCard(hsd);
	status = (bsp_status_t) HAL_SD_ConfigWideBusOperation(hsd, SDIO_BUS_WIDE_1B);

	return status;
}

uint32_t bsp_sd_get_error(bsp_dev_sdio_t dev_num)
{
	SD_HandleTypeDef* hsd;
	hsd = &sd_handle[dev_num];

	return hsd->ErrorCode;
}
/**
  * @brief  De-initialize the SD comunication bus
  * @param  dev_num: SD dev num.
  * @retval status: status of the deinit.
  */
bsp_status_t bsp_sd_deinit(bsp_dev_sdio_t dev_num)
{
	bsp_status_t status;

	/* De-initialize the SD comunication bus */
	status = (bsp_status_t) bsp_sdio_deinit(dev_num);

	return status;
}

uint32_t *bsp_sd_get_cid(bsp_dev_sdio_t dev_num)
{
	SD_HandleTypeDef* hsd;

	hsd = &sd_handle[dev_num];

	return hsd->CID;
}

uint32_t *bsp_sd_get_csd(bsp_dev_sdio_t dev_num)
{
	SD_HandleTypeDef* hsd;

	hsd = &sd_handle[dev_num];

	return hsd->CSD;
}

uint32_t bsp_sd_get_state(bsp_dev_sdio_t dev_num)
{
	SD_HandleTypeDef* hsd;

	hsd = &sd_handle[dev_num];

	return HAL_SD_GetCardState(hsd);
}

bsp_status_t bsp_sd_change_bus_width(bsp_dev_sdio_t dev_num, uint8_t bus_size)
{
	SD_HandleTypeDef* hsd;
	bsp_status_t status;

	hsd = &sd_handle[dev_num];

	switch(bus_size) {
	case 1:
		status = (bsp_status_t) HAL_SD_ConfigWideBusOperation(hsd, SDIO_BUS_WIDE_1B);
		break;
	case 4:
		status = (bsp_status_t) HAL_SD_ConfigWideBusOperation(hsd, SDIO_BUS_WIDE_4B);
		break;
	/* Unused, pins are not present on Hydrabus v1
	case 8:
		status = (bsp_status_t) HAL_SD_ConfigWideBusOperation(hsd, SDIO_BUS_WIDE_8B);
		break;
	*/
	default:
		status = BSP_ERROR;
	}
	return status;
}

bsp_status_t bsp_sd_read_extcsd(bsp_dev_sdio_t dev_num, uint8_t * extcsd)
{
	SD_HandleTypeDef* hsd;
	hsd = &sd_handle[dev_num];
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
	(void)SDIO_ConfigData(hsd->Instance, &config);

	status = (bsp_status_t)SDIO_SendCommand(hsd->Instance, &cmd);
	if(status != BSP_OK) {
		return status;
	}

	tickstart = HAL_GetTick();
	while (!__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND)) {
		if(__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_RXFIFOHF) && (dataremaining > 0U)) {
			for(count = 0U; count < 8U; count++) {
				data = SDIO_ReadFIFO(hsd->Instance);
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

		if (((HAL_GetTick() - tickstart) >= SD_TIMEOUT_MAX)) {
			__HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
			return BSP_TIMEOUT;
		}
	}
	__HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
	return BSP_OK;
}

bsp_status_t bsp_sd_get_info(bsp_dev_sdio_t dev_num, bsp_sd_info_t * sd_info)
{
	SD_HandleTypeDef* hsd;

	hsd = &sd_handle[dev_num];

	HAL_SD_GetCardInfo(hsd, (HAL_SD_CardInfoTypeDef *)sd_info);

	return BSP_OK;
}

/**
  * @brief  Sends a Byte in blocking mode and return the status.
  * @param  dev_num: SD dev num.
  * @param  tx_data: data to send.
  * @param  nb_data: Number of data to send.
  * @retval status of the transfer.
  */
bsp_status_t bspsd_write_block(bsp_dev_sdio_t dev_num, uint8_t* tx_data, uint32_t block_number)
{
	SD_HandleTypeDef* hsd;
	bsp_status_t status;

	hsd = &sd_handle[dev_num];

	status = (bsp_status_t) HAL_SD_WriteBlocks(hsd, tx_data, block_number, 1, SD_TIMEOUT_MAX);

	return status;
}

/**
  * @brief  Read a Byte in blocking mode and return the status.
  * @param  dev_num: SD dev num.
  * @param  rx_data: Data to receive.
  * @param  nb_data: Number of data to receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_sd_read_block(bsp_dev_sdio_t dev_num, uint8_t* rx_data, uint32_t block_number)
{
	SD_HandleTypeDef* hsd;
	bsp_status_t status;

	hsd = &sd_handle[dev_num];

	status = (bsp_status_t) HAL_SD_ReadBlocks(hsd, rx_data, block_number, 1, SD_TIMEOUT_MAX);

	return status;
}
