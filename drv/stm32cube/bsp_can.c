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
#include "stm32f405xx.h"

/*
Warning in order to use this driver all GPIOs peripherals shall be enabled.
*/
#define CANx_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_CAN (BSP_DEV_CAN_END)

/* For whatever reason, this flag is not in stm32f4xx_hal_can.h */
#define CAN_FLAG_FMP0 ((uint32_t)0x12000003)

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

		/* Disable peripherals GPIO */
		HAL_GPIO_DeInit(BSP_CAN1_TX_PORT, BSP_CAN1_TX_PIN);
		HAL_GPIO_DeInit(BSP_CAN1_RX_PORT, BSP_CAN1_RX_PIN);
	} else {
		/* Reset peripherals */
		__CAN2_FORCE_RESET();
		__CAN2_RELEASE_RESET();

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
		/* Re-Initialize the CAN comunication bus */
		bsp_can_init(dev_num, can_mode_conf[dev_num]);
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
	status = HAL_CAN_Init(hcan);

	return status;
}

uint32_t bsp_can_get_speed(bsp_dev_can_t dev_num)
{
	CAN_HandleTypeDef* hcan;
	hcan = &can_handle[dev_num];
	return 2000000/hcan->Init.Prescaler;
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
	hcan->Init.TTCM = DISABLE;

	/* automatic bus-off management */
	hcan->Init.ABOM = ENABLE;

	/* automatic wake-up mode */
	hcan->Init.AWUM = ENABLE;

	/* non-automatic retransmission mode */
	hcan->Init.NART = DISABLE;

	/* receive FIFO Locked mode */
	hcan->Init.RFLM = DISABLE;

	/* transmit FIFO priority */
	hcan->Init.TXFP = DISABLE;

	hcan->Init.SJW  = CAN_SJW_1TQ;
	hcan->Init.Mode = CAN_MODE_NORMAL;

	/* CAN Baudrate */
	hcan->Init.BS1 = CAN_BS1_14TQ;
	hcan->Init.BS2 = CAN_BS2_6TQ;

	//hcan->Init.Prescaler=1;        // 2000 kbit/s
	//hcan->Init.Prescaler=2;        // 1000 kbit/s
	hcan->Init.Prescaler=4;        //  500 kbit/s
	//hcan->Init.Prescaler=5;        //  400 kbit/s
	//hcan->Init.Prescaler=8;        //  250 kbit/s
	//hcan->Init.Prescaler=10;       //  200 kbit/s
	//hcan->Init.Prescaler=16;       //  125 kbit/s
	//hcan->Init.Prescaler=20;       //  100 kbit/s
	//hcan->Init.Prescaler=40;       //   50 kbit/s
	//hcan->Init.Prescaler=80;       //   40 kbit/s
	//hcan->Init.Prescaler=200;      //   10 kbit/s

	status = HAL_CAN_Init(hcan);

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
	CAN_FilterConfTypeDef hcanfilter;
	CAN_HandleTypeDef* hcan;

	can_mode_conf[dev_num] = mode_conf;
	hcan = &can_handle[dev_num];

	bsp_status_t status;

	can_gpio_hw_init(dev_num);

	hcanfilter.FilterIdLow = 0;
	hcanfilter.FilterIdHigh = 0;
	hcanfilter.FilterMaskIdHigh = 0;
	hcanfilter.FilterMaskIdLow = 0;
	hcanfilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	hcanfilter.FilterNumber = 14*dev_num;
	hcanfilter.FilterMode = CAN_FILTERMODE_IDMASK;
	hcanfilter.FilterScale = CAN_FILTERSCALE_16BIT;
	hcanfilter.FilterActivation = ENABLE;
	hcanfilter.BankNumber = 14;

	status = HAL_CAN_ConfigFilter(hcan, &hcanfilter);

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
	CAN_FilterConfTypeDef hcanfilter;
	CAN_HandleTypeDef* hcan;

	can_mode_conf[dev_num] = mode_conf;
	hcan = &can_handle[dev_num];

	bsp_status_t status;

	can_gpio_hw_init(dev_num);

	hcanfilter.FilterIdLow = id_low<<5;
	hcanfilter.FilterIdHigh = id_high<<5;
	hcanfilter.FilterMaskIdHigh = 0;
	hcanfilter.FilterMaskIdLow = 0;
	hcanfilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	hcanfilter.FilterNumber = 14*dev_num;
	hcanfilter.FilterMode = CAN_FILTERMODE_IDLIST;
	hcanfilter.FilterScale = CAN_FILTERSCALE_16BIT;
	hcanfilter.FilterActivation = ENABLE;
	hcanfilter.BankNumber = 14;

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
bsp_status_t bsp_can_write(bsp_dev_can_t dev_num, CanTxMsgTypeDef* tx_msg)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	hcan->pTxMsg = tx_msg;

	status = HAL_CAN_Transmit(hcan, CANx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		can_error(dev_num);
	}
	return status;
}

/**
  * @brief  Read a message in blocking mode and return the status.
  * @param  dev_num: CAN dev num.
  * @param  rx_msg: Message to receive.
  * @retval status of the transfer.
  */
bsp_status_t bsp_can_read(bsp_dev_can_t dev_num, CanRxMsgTypeDef* rx_msg)
{
	CAN_HandleTypeDef* hcan;
	bsp_status_t status;

	hcan = &can_handle[dev_num];

	hcan->pRxMsg = rx_msg;

	status = HAL_CAN_Receive(hcan, CAN_FIFO0, CANx_TIMEOUT_MAX);
	if(status != BSP_OK) {
		can_error(dev_num);
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

	return __HAL_CAN_MSG_PENDING(hcan, CAN_FIFO0);
}

