/*
HydraBus/HydraNFC - Copyright (C) 2015 Nicolas OBERLI

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
#include "bsp_can.h"
#include "bsp_can_conf.h"
#include "stm32.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define CANx_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_CAN (BSP_DEV_CAN_END)

static CAN_HandleTypeDef can_handle[NB_CAN];
static mode_config_proto_t* can_mode_conf[NB_CAN];

/**
  * @brief  Init low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: CAN dev num
  * @retval None
  */
static void can_gpio_hw_init(bsp_dev_can_t dev_num)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	__CAN1_CLK_ENABLE();

	if(dev_num == BSP_DEV_CAN1) {
		/* Enable the CAN peripheral */

		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull  = GPIO_NOPULL;
		GPIO_InitStructure.Speed = BSP_CAN1_GPIO_SPEED;
		GPIO_InitStructure.Alternate = BSP_CAN1_AF;

		/* CAN1 TX pin configuration */
		GPIO_InitStructure.Pin = BSP_CAN1_TX_PIN;
		HAL_GPIO_Init(BSP_CAN1_TX_PORT, &GPIO_InitStructure);

		/* CAN1 RX pin configuration */
		GPIO_InitStructure.Pin = BSP_CAN1_RX_PIN;
		HAL_GPIO_Init(BSP_CAN1_RX_PORT, &GPIO_InitStructure);

	} else {
		/* Enable the CAN peripheral */
		__CAN2_CLK_ENABLE();

		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull  = GPIO_NOPULL;
		GPIO_InitStructure.Speed = BSP_CAN2_GPIO_SPEED;
		GPIO_InitStructure.Alternate = BSP_CAN2_AF;

		/* CAN2 TX pin configuration */
		GPIO_InitStructure.Pin = BSP_CAN2_TX_PIN;
		HAL_GPIO_Init(BSP_CAN2_TX_PORT, &GPIO_InitStructure);

		/* CAN2 RX pin configuration */
		GPIO_InitStructure.Pin = BSP_CAN2_RX_PIN;
		HAL_GPIO_Init(BSP_CAN2_RX_PORT, &GPIO_InitStructure);
	}

}

/**
  * @brief  DeInit low level hardware: GPIO, CLOCK, NVIC...
  * @param  dev_num: CAN dev num
  * @retval None
  */
static void can_gpio_hw_deinit(bsp_dev_can_t dev_num)
{
	if(dev_num == BSP_DEV_CAN1) {
		/* Reset peripherals */
		__CAN1_FORCE_RESET();
		__CAN1_RELEASE_RESET();
		__CAN1_CLK_DISABLE();

		/* Disable peripherals GPIO */
		HAL_GPIO_DeInit(BSP_CAN1_TX_PORT, BSP_CAN1_TX_PIN);
		HAL_GPIO_DeInit(BSP_CAN1_RX_PORT, BSP_CAN1_RX_PIN);
	} else {
		/* Reset peripherals */
		__CAN2_FORCE_RESET();
		__CAN2_RELEASE_RESET();
		__CAN2_CLK_DISABLE();

		/* Disable peripherals GPIO */
		HAL_GPIO_DeInit(BSP_CAN2_TX_PORT, BSP_CAN2_TX_PIN);
		HAL_GPIO_DeInit(BSP_CAN2_RX_PORT, BSP_CAN2_RX_PIN);
	}
}

/**
  * @brief  CANx error treatment function.
  * @param  dev_num: CAN dev num
  * @retval None
  */
static void can_error(bsp_dev_can_t dev_num)
{
	if(bsp_can_deinit(dev_num) == BSP_OK) {
		/* Re-Initialize the CAN comunication
		 * bus */
		can_gpio_hw_init(dev_num);
		bsp_can_init(dev_num, can_mode_conf[dev_num]);
		bsp_can_init_filter(dev_num, can_mode_conf[dev_num]);
	}
}

/**
  * @brief  CANx bus speed setting
  * @param  dev_num: CAN dev num
  * @param  speed: CAN dev speed in bps
  * @retval status: status of the init.
  */
bsp_status_t bsp_can_set_speed(bsp_dev_can_t dev_num, uint32_t speed)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	HAL_CAN_DeInit(hcan);

	hcan->Init.Prescaler = 2000000/speed;
	HAL_CAN_Stop(hcan);
	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	return status;
}

uint32_t bsp_can_get_speed(bsp_dev_can_t dev_num)
{
	CAN_HandleTypeDef* hcan;
	hcan = &can_handle[dev_num];
	return 2000000/hcan->Init.Prescaler;
}

/**
  * @brief  CANx bus timing settings
  * @param  dev_num: CAN dev num
  * @param  ts1: Time setting 1
  * @param  ts2: Time setting 2
  * @param  sjw: Resynchronization Jump Width
  * @retval status: status of the init.
  */
bsp_status_t bsp_can_set_timings(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	HAL_CAN_DeInit(hcan);

	hcan->Init.TimeSeg1 = mode_conf->config.can.dev_timing&0xf0000;
	hcan->Init.TimeSeg2 = mode_conf->config.can.dev_timing&0x700000;
	hcan->Init.SyncJumpWidth  = mode_conf->config.can.dev_timing&0x3000000;

	HAL_CAN_Stop(hcan);
	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	return status;
}

bsp_status_t bsp_can_set_ts1(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint8_t ts1)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	HAL_CAN_DeInit(hcan);

	hcan->Init.TimeSeg1 = (uint32_t)(ts1-1)<<16;
	HAL_CAN_Stop(hcan);
	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	mode_conf->config.can.dev_timing = bsp_can_get_timings(dev_num);

	return status;
}

bsp_status_t bsp_can_set_ts2(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint8_t ts2)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	HAL_CAN_DeInit(hcan);

	hcan->Init.TimeSeg2 = (uint32_t)(ts2-1)<<20;
	HAL_CAN_Stop(hcan);
	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	mode_conf->config.can.dev_timing = bsp_can_get_timings(dev_num);

	return status;
}

bsp_status_t bsp_can_set_sjw(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint8_t sjw)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	HAL_CAN_DeInit(hcan);

	hcan->Init.SyncJumpWidth = (uint32_t)(sjw-1)<<24;
	HAL_CAN_Stop(hcan);
	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	mode_conf->config.can.dev_timing = bsp_can_get_timings(dev_num);

	return status;
}

uint32_t bsp_can_get_timings(bsp_dev_can_t dev_num)
{
	CAN_HandleTypeDef* hcan;
	hcan = &can_handle[dev_num];
	return hcan->Instance->BTR;
}

bsp_status_t bsp_can_mode_rw(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	HAL_CAN_DeInit(hcan);

	mode_conf->config.can.dev_mode = BSP_CAN_MODE_RW;
	hcan->Init.Mode = CAN_MODE_NORMAL;

	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	return status;
}


/**
  * @brief  Init CAN device.
  * @param  dev_num: CAN dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_can_init(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	can_mode_conf[dev_num] = mode_conf;
	hcan = &can_handle[dev_num];

	can_gpio_hw_init(dev_num);

	if(dev_num == BSP_DEV_CAN1) {
		hcan->Instance = BSP_CAN1;
		HAL_CAN_DeInit(hcan);
	} else { /* CAN2 */
		hcan->Instance = BSP_CAN2;
		HAL_CAN_DeInit(hcan);
	}

	__HAL_CAN_RESET_HANDLE_STATE(hcan);

	/* CAN cell init */

	/* time triggered communication mode */
	hcan->Init.TimeTriggeredMode = DISABLE;

	/* automatic bus-off management */
	hcan->Init.AutoBusOff = ENABLE;

	/* automatic wake-up mode */
	hcan->Init.AutoWakeUp = ENABLE;

	/* non-automatic retransmission mode */
	hcan->Init.AutoRetransmission = ENABLE;

	/* receive FIFO Locked mode */
	hcan->Init.ReceiveFifoLocked = DISABLE;

	/* transmit FIFO priority */
	hcan->Init.TransmitFifoPriority = DISABLE;

	if(mode_conf->config.can.dev_mode == BSP_CAN_MODE_RO) {
		hcan->Init.Mode = CAN_MODE_SILENT;
	} else {
		hcan->Init.Mode = CAN_MODE_NORMAL;
	}

	/* CAN timing values */
	hcan->Init.TimeSeg1 = mode_conf->config.can.dev_timing&0xf0000;
	hcan->Init.TimeSeg2 = mode_conf->config.can.dev_timing&0x700000;
	hcan->Init.SyncJumpWidth  = mode_conf->config.can.dev_timing&0x3000000;

	/* CAN Baudrate */
	hcan->Init.Prescaler = 2000000/mode_conf->config.can.dev_speed;

	HAL_CAN_Stop(hcan);
	status = HAL_CAN_Init(hcan);
	HAL_CAN_Start(hcan);

	return status;
}

/**
  * @brief  Init CAN device filter to capture all
  * @param  dev_num: CAN dev num.
  * @param  mode_conf: Mode config proto.
  * @retval status: status of the init.
  */
bsp_status_t bsp_can_init_filter(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf)
{
	CAN_FilterTypeDef hcanfilter;
	CAN_HandleTypeDef* hcan;

	can_mode_conf[dev_num] = mode_conf;
	hcan = &can_handle[dev_num];

	bsp_status_t status;

	hcanfilter.FilterIdLow = 0;
	hcanfilter.FilterIdHigh = 0;
	hcanfilter.FilterMaskIdHigh = 0;
	hcanfilter.FilterMaskIdLow = 0;
	hcanfilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	hcanfilter.FilterBank = 14*dev_num;
	hcanfilter.FilterMode = CAN_FILTERMODE_IDMASK;
	hcanfilter.FilterScale = CAN_FILTERSCALE_16BIT;
	hcanfilter.FilterActivation = ENABLE;
	hcanfilter.SlaveStartFilterBank = 14;

	HAL_CAN_Stop(hcan);
	status = HAL_CAN_ConfigFilter(hcan, &hcanfilter);
	HAL_CAN_Start(hcan);

	return status;
}

/**
  * @brief  Set CAN device filter by ID
  * @param  dev_num: CAN dev num.
  * @param  mode_conf: Mode config proto.
  * @param  id_low: Lower ID to capture
  * @param  id_high: Higher ID to capture
  * @retval status: status of the init.
  */
bsp_status_t bsp_can_set_filter(bsp_dev_can_t dev_num,
				mode_config_proto_t* mode_conf,
				uint32_t id_low, uint32_t id_high)
{
	CAN_FilterTypeDef hcanfilter;
	CAN_HandleTypeDef* hcan;

	can_mode_conf[dev_num] = mode_conf;
	hcan = &can_handle[dev_num];

	bsp_status_t status;

	hcanfilter.FilterIdLow = id_low<<5;
	hcanfilter.FilterIdHigh = id_high<<5;
	hcanfilter.FilterMaskIdHigh = 0;
	hcanfilter.FilterMaskIdLow = 0;
	hcanfilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	hcanfilter.FilterBank = 14*dev_num;
	hcanfilter.FilterMode = CAN_FILTERMODE_IDLIST;
	hcanfilter.FilterScale = CAN_FILTERSCALE_16BIT;
	hcanfilter.FilterActivation = ENABLE;
	hcanfilter.SlaveStartFilterBank = 14;

	status = HAL_CAN_ConfigFilter(hcan, &hcanfilter);

	return status;
}

/**
  * @brief  De-initialize the CAN comunication bus
  * @param  dev_num: CAN dev num.
  * @retval status: status of the deinit.
  */
bsp_status_t bsp_can_deinit(bsp_dev_can_t dev_num)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	/* Stop the CAN controller */
	HAL_CAN_Stop(hcan);

	/* De-initialize the CAN comunication bus */
	status = HAL_CAN_DeInit(hcan);

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	can_gpio_hw_deinit(dev_num);

	return status;
}

/**
  * @brief  Sends a message in blocking mode and return the status.
  * @param  dev_num: CAN dev num.
  * @param  tx_msg: Message to send
  * @retval status of the transfer.
  */
bsp_status_t bsp_can_write(bsp_dev_can_t dev_num, can_tx_frame* tx_msg)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;
	uint32_t dummy;
	uint32_t start_time;

	hcan = &can_handle[dev_num];

	start_time = HAL_GetTick();

	while(HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0) {
		if((HAL_GetTick()-start_time) > CANx_TIMEOUT_MAX) {
			return BSP_TIMEOUT;
		}
	}
	status = HAL_CAN_AddTxMessage(hcan, &(tx_msg->header), tx_msg->data, &dummy);

	switch(status) {
	case BSP_ERROR:
		can_error(dev_num);
		break;
	case BSP_OK:
	case BSP_TIMEOUT:
	case BSP_BUSY:
	default:
		return status;
	}
	return status;
}

/**
  * @brief  Read a message in blocking mode and return the status.
  * @param  dev_num: CAN dev num.
  * @param  rx_msg: Message to receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_can_read(bsp_dev_can_t dev_num, can_rx_frame* rx_msg)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;
	uint32_t start_time;

	hcan = &can_handle[dev_num];

	start_time = HAL_GetTick();
	while(HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) == 0) {
		if((HAL_GetTick()-start_time) > CANx_TIMEOUT_MAX) {
			return BSP_TIMEOUT;
		}
	}
	status = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &(rx_msg->header), rx_msg->data);
	switch(status) {
	case BSP_ERROR:
		can_error(dev_num);
		break;
	case BSP_OK:
	case BSP_TIMEOUT:
	case BSP_BUSY:
	default:
		return status;
	}
	return status;
}

/**
  * @brief  Checks if the CAN receive buffer is empty
  * @retval Number of messages in the FIFO
  */
bsp_status_t bsp_can_rxne(bsp_dev_can_t dev_num)
{
	CAN_HandleTypeDef* hcan;
	hcan = &can_handle[dev_num];

	return HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0);
}
