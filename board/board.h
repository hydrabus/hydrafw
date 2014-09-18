/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
    HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for HydraBus board (STM32F4 LQFP64).
 */

/*
 * Board identifier.
 */
#define BOARD_HYDRABUS
#define BOARD_NAME    "HydraBus 1.0"


/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK      0
#endif

#if !defined(STM32_HSECLK)
#define STM32_HSECLK      8000000
#endif


/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD         300

/*
 * MCU type as defined in the ST header.
 */
#define STM32F40_41xxx

/*
 * IO pins assignments.
 */
#define GPIOA_UBTN        0
#define GPIOA_PIN1        1
#define GPIOA_PIN2        2
#define GPIOA_PIN3        3
#define GPIOA_ULED        4
#define GPIOA_PIN5        5
#define GPIOA_PIN6        6
#define GPIOA_PIN7        7
#define GPIOA_PIN8        8
#define GPIOA_PIN9        9
#define GPIOA_PIN10       10
#define GPIOA_OTG_FS_DM   11
#define GPIOA_OTG_FS_DP   12
#define GPIOA_SWDIO       13
#define GPIOA_SWCLK       14
#define GPIOA_PIN15       15

#define GPIOB_PIN0        0
#define GPIOB_PIN1        1
#define GPIOB_PIN2        2
#define GPIOB_PIN3        3
#define GPIOB_PIN4        4
#define GPIOB_PIN5        5
#define GPIOB_PIN6        6
#define GPIOB_PIN7        7
#define GPIOB_PIN8        8
#define GPIOB_PIN9        9
#define GPIOB_PIN10       10
#define GPIOB_PIN11       11
#define GPIOB_OTG_HS_ID   12
#define GPIOB_OTG_HS_VBUS 13
#define GPIOB_OTG_HS_DM   14
#define GPIOB_OTG_HS_DP   15

#define GPIOC_PIN0        0
#define GPIOC_PIN1        1
#define GPIOC_PIN2        2
#define GPIOC_PIN3        3
#define GPIOC_PIN4        4
#define GPIOC_PIN5        5
#define GPIOC_PIN6        6
#define GPIOC_PIN7        7
#define GPIOC_SDIO_D0     8
#define GPIOC_SDIO_D1     9
#define GPIOC_SDIO_D2     10
#define GPIOC_SDIO_D3     11
#define GPIOC_SDIO_CK     12
#define GPIOC_PIN13       13
#define GPIOC_PIN14       14
#define GPIOC_PIN15       15

#define GPIOD_SDIO_CMD    2

#define GPIOH_OSC_IN      0
#define GPIOH_OSC_OUT     1

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 *
 * 1 for open drain outputs denotes hi-Z state
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_2M(n)            (0U << ((n) * 2))
#define PIN_OSPEED_25M(n)           (1U << ((n) * 2))
#define PIN_OSPEED_50M(n)           (2U << ((n) * 2))
#define PIN_OSPEED_100M(n)          (3U << ((n) * 2))
#define PIN_PUDR_FLOATING(n)        (0U << ((n) * 2))
#define PIN_PUDR_PULLUP(n)          (1U << ((n) * 2))
#define PIN_PUDR_PULLDOWN(n)        (2U << ((n) * 2))
#define PIN_AFIO_AF(n, v)           ((v##U) << ((n % 8) * 4))

/*
 * GPIOA setup:
 *
 * PA0  - BUTTON                    (input floating).
 * PA1  - PIN1                      (input pullup).
 * PA2  - PIN2                      (input pullup).
 * PA3  - PIN3                      (input pullup).
 * PA4  - PIN4                      (input pullup).
 * PA5  - PIN5                      (input pullup).
 * PA6  - PIN6                      (input pullup).
 * PA7  - PIN7                      (input pullup).
 * PA8  - PIN8                      (input pullup).
 * PA9  - PIN9                      (input pullup).
 * PA10 - PIN10                     (input pullup).
 * PA11 - OTG_FS_DM                 (alternate 10).
 * PA12 - OTG_FS_DP                 (alternate 10).
 * PA13 - SWDIO                     (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - PIN15                     (input pullup).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_INPUT(GPIOA_UBTN) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN3) |           \
                                     PIN_MODE_OUTPUT(GPIOA_ULED) |          \
                                     PIN_MODE_INPUT(GPIOA_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOA_OTG_FS_DM) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_OTG_FS_DP) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_INPUT(GPIOA_PIN15))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_UBTN) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ULED) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_OTG_FS_DM) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_OTG_FS_DP) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN15))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_100M(GPIOA_UBTN) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN1) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN2) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN3) |          \
                                     PIN_OSPEED_100M(GPIOA_ULED) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN5) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN6) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN7) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN8) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN9) |          \
                                     PIN_OSPEED_100M(GPIOA_PIN10) |         \
                                     PIN_OSPEED_100M(GPIOA_OTG_FS_DM) |     \
                                     PIN_OSPEED_100M(GPIOA_OTG_FS_DP) |     \
                                     PIN_OSPEED_100M(GPIOA_SWDIO) |         \
                                     PIN_OSPEED_100M(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_100M(GPIOA_PIN15))
#define VAL_GPIOA_PUPDR             (PIN_PUDR_FLOATING(GPIOA_UBTN) |        \
                                     PIN_PUDR_PULLUP(GPIOA_PIN1) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN2) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN3) |          \
                                     PIN_PUDR_PULLUP(GPIOA_ULED) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN5) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN6) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN7) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN8) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN9) |          \
                                     PIN_PUDR_PULLUP(GPIOA_PIN10) |         \
                                     PIN_PUDR_FLOATING(GPIOA_OTG_FS_DM) |   \
                                     PIN_PUDR_FLOATING(GPIOA_OTG_FS_DP) |   \
                                     PIN_PUDR_FLOATING(GPIOA_SWDIO) |       \
                                     PIN_PUDR_FLOATING(GPIOA_SWCLK) |       \
                                     PIN_PUDR_PULLUP(GPIOA_PIN15))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(GPIOA_UBTN) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN3) |             \
                                     PIN_ODR_LOW(GPIOA_ULED) |              \
                                     PIN_ODR_HIGH(GPIOA_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOA_OTG_FS_DM) |        \
                                     PIN_ODR_HIGH(GPIOA_OTG_FS_DP) |        \
                                     PIN_ODR_HIGH(GPIOA_SWDIO) |            \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |            \
                                     PIN_ODR_HIGH(GPIOA_PIN15))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_UBTN, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN1, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOA_ULED, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN7, 0))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOA_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOA_OTG_FS_DM, 10) |     \
                                     PIN_AFIO_AF(GPIOA_OTG_FS_DP, 10) |     \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0) |          \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0) |          \
                                     PIN_AFIO_AF(GPIOA_PIN15, 0))

/*
 * GPIOB setup:
 *
 * PB0  - PIN0                      (input pullup).
 * PB1  - PIN1                      (input pullup).
 * PB2  - PIN2                      (input pullup).
 * PB3  - PIN3                      (input pullup).
 * PB4  - PIN4                      (input pullup).
 * PB5  - PIN5                      (input pullup).
 * PB6  - PIN6                      (input pullup).
 * PB7  - PIN7                      (input pullup).
 * PB8  - PIN8                      (input pullup).
 * PB9  - PIN9                      (input pullup).
 * PB10 - PIN10                     (input pullup).
 * PB11 - PIN11                     (input pullup).
 * PB12 - OTG_HS_ID                 (input pullup).
 * PB13 - OTG_HS_VBUS               (input pullup).
 * PB14 - OTG_HS_DM                 (input pullup).
 * PB15 - OTG_HS_DP                 (input pullup).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_INPUT(GPIOB_PIN0) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN1) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN2) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN3) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN4) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN5) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN6) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN7) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN8) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN9) |            \
                                     PIN_MODE_INPUT(GPIOB_PIN10) |           \
                                     PIN_MODE_INPUT(GPIOB_PIN11) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_OTG_HS_ID) |   \
                                     PIN_MODE_INPUT(GPIOB_OTG_HS_VBUS) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_OTG_HS_DM) |   \
                                     PIN_MODE_ALTERNATE(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_PIN0) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN1) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN2) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN3) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN4) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN5) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN6) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN7) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN8) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN9) |        \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN10) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN11) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_ID) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_VBUS) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_DM) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_100M(GPIOB_PIN0) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN1) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN2) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN3) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN4) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN5) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN6) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN7) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN8) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN9) |           \
                                     PIN_OSPEED_100M(GPIOB_PIN10) |          \
                                     PIN_OSPEED_100M(GPIOB_PIN11) |          \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_ID) |      \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_VBUS) |    \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_DM) |      \
                                     PIN_OSPEED_100M(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_PUPDR             (PIN_PUDR_PULLUP(GPIOB_PIN0) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN1) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN2) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN3) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN4) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN5) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN6) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN7) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN8) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN9) |          \
                                     PIN_PUDR_PULLUP(GPIOB_PIN10) |         \
                                     PIN_PUDR_PULLUP(GPIOB_PIN11) |         \
                                     PIN_PUDR_FLOATING(GPIOB_OTG_HS_ID) |   \
                                     PIN_PUDR_FLOATING(GPIOB_OTG_HS_VBUS) | \
                                     PIN_PUDR_FLOATING(GPIOB_OTG_HS_DM) |   \
                                     PIN_PUDR_FLOATING(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_ODR               (PIN_ODR_HIGH(GPIOB_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_ID) |        \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_VBUS) |      \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_DM) |        \
                                     PIN_ODR_HIGH(GPIOB_OTG_HS_DP))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_PIN0, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN1, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN7, 0))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_PIN8, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN9, 0) |           \
                                     PIN_AFIO_AF(GPIOB_PIN10, 0) |          \
                                     PIN_AFIO_AF(GPIOB_PIN11, 0) |          \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_ID, 12) |     \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_VBUS, 0) |    \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_DM, 12) |     \
                                     PIN_AFIO_AF(GPIOB_OTG_HS_DP, 12))

/*
 * GPIOC setup:
 *
 * PC0  - PIN0                      (input pullup).
 * PC1  - PIN1                      (input pullup).
 * PC2  - PIN2                      (input pullup).
 * PC3  - PIN3                      (input pullup).
 * PC4  - PIN4                      (input pullup).
 * PC5  - PIN5                      (input pullup).
 * PC6  - PIN6                      (input pullup).
 * PC7  - PIN7                      (input pullup).
 * PC8  - SDIO D0.
 * PC9  - SDIO D1.
 * PC10 - SDIO D2.
 * PC11 - SDIO D3.
 * PC12 - SDIO CK.
 * PC13 - PIN13                     (input pullup).
 * PC14 - PIN14                     (input pullup).
 * PC15 - PIN15                     (input pullup).
 */

#define VAL_GPIOC_MODER             (PIN_MODE_INPUT(GPIOC_PIN0) |\
                                     PIN_MODE_INPUT(GPIOC_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN7) |           \
                                     PIN_MODE_ALTERNATE(GPIOC_SDIO_D0) |    \
                                     PIN_MODE_ALTERNATE(GPIOC_SDIO_D1) |    \
                                     PIN_MODE_ALTERNATE(GPIOC_SDIO_D2) |    \
                                     PIN_MODE_ALTERNATE(GPIOC_SDIO_D3) |    \
                                     PIN_MODE_ALTERNATE(GPIOC_SDIO_CK) |    \
                                     PIN_MODE_INPUT(GPIOC_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN15))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_PIN0) |\
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN15))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_100M(GPIOC_PIN0) |\
                                     PIN_OSPEED_100M(GPIOC_PIN1) |          \
                                     PIN_OSPEED_100M(GPIOC_PIN2) |          \
                                     PIN_OSPEED_100M(GPIOC_PIN3) |          \
                                     PIN_OSPEED_100M(GPIOC_PIN4) |          \
                                     PIN_OSPEED_100M(GPIOC_PIN5) |          \
                                     PIN_OSPEED_100M(GPIOC_PIN6) |          \
                                     PIN_OSPEED_100M(GPIOC_PIN7) |          \
                                     PIN_OSPEED_25M(GPIOC_SDIO_D0) |        \
                                     PIN_OSPEED_25M(GPIOC_SDIO_D1) |        \
                                     PIN_OSPEED_25M(GPIOC_SDIO_D2) |        \
                                     PIN_OSPEED_25M(GPIOC_SDIO_D3) |        \
                                     PIN_OSPEED_25M(GPIOC_SDIO_CK) |        \
                                     PIN_OSPEED_100M(GPIOC_PIN13) |         \
                                     PIN_OSPEED_100M(GPIOC_PIN14) |         \
                                     PIN_OSPEED_100M(GPIOC_PIN15))
#define VAL_GPIOC_PUPDR             (PIN_PUDR_PULLUP(GPIOC_PIN0) |\
                                     PIN_PUDR_PULLUP(GPIOC_PIN1) |         \
                                     PIN_PUDR_PULLUP(GPIOC_PIN2) |         \
                                     PIN_PUDR_PULLUP(GPIOC_PIN3) |         \
                                     PIN_PUDR_PULLUP(GPIOC_PIN4) |         \
                                     PIN_PUDR_PULLUP(GPIOC_PIN5) |         \
                                     PIN_PUDR_PULLUP(GPIOC_PIN6) |         \
                                     PIN_PUDR_PULLUP(GPIOC_PIN7) |         \
                                     PIN_PUDR_PULLUP(GPIOC_SDIO_D0) |       \
                                     PIN_PUDR_PULLUP(GPIOC_SDIO_D1) |       \
                                     PIN_PUDR_PULLUP(GPIOC_SDIO_D2) |       \
                                     PIN_PUDR_PULLUP(GPIOC_SDIO_D3) |       \
                                     PIN_PUDR_PULLUP(GPIOC_SDIO_CK) |       \
                                     PIN_PUDR_PULLUP(GPIOC_PIN13) |        \
                                     PIN_PUDR_PULLUP(GPIOC_PIN14) |        \
                                     PIN_PUDR_PULLUP(GPIOC_PIN15))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_PIN0) |\
                                     PIN_ODR_HIGH(GPIOC_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN15))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_PIN0, 0) |\
                                     PIN_AFIO_AF(GPIOC_PIN1, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN2, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN3, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN4, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN5, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN6, 0) |           \
                                     PIN_AFIO_AF(GPIOC_PIN7, 0))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_SDIO_D0, 12) |\
                                     PIN_AFIO_AF(GPIOC_SDIO_D1, 12) |       \
                                     PIN_AFIO_AF(GPIOC_SDIO_D2, 12) |       \
                                     PIN_AFIO_AF(GPIOC_SDIO_D3, 12) |       \
                                     PIN_AFIO_AF(GPIOC_SDIO_CK, 12) |       \
                                     PIN_AFIO_AF(GPIOC_PIN13, 0) |          \
                                     PIN_AFIO_AF(GPIOC_PIN14, 0) |          \
                                     PIN_AFIO_AF(GPIOC_PIN15, 0))

/*
 * GPIOD setup:
 *
 * PD2  - PIN2 GPIOD_SDIO_CMD.
 */

#define VAL_GPIOD_MODER             (PIN_MODE_ALTERNATE(GPIOD_SDIO_CMD))
#define VAL_GPIOD_OTYPER            (0x00000000)
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_25M(GPIOD_SDIO_CMD))
#define VAL_GPIOD_PUPDR             (PIN_PUDR_PULLUP(GPIOD_SDIO_CMD))
#define VAL_GPIOD_ODR               (0x00000000)
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(GPIOD_SDIO_CMD, 12))
#define VAL_GPIOD_AFRH              (0x00000000)

/*
 * GPIOE setup:
 * No GPIOE on STM32F4 LQFP64
 */
#define VAL_GPIOE_MODER             (0x00000000)
#define VAL_GPIOE_OTYPER            (0x00000000)
#define VAL_GPIOE_OSPEEDR           (0x00000000)
#define VAL_GPIOE_PUPDR             (0x00000000)
#define VAL_GPIOE_ODR               (0x00000000)
#define VAL_GPIOE_AFRL              (0x00000000)
#define VAL_GPIOE_AFRH              (0x00000000)

/*
 * GPIOF setup:
 * No GPIOF on STM32F4 LQFP64
 */
#define VAL_GPIOF_MODER             (0x00000000)
#define VAL_GPIOF_OTYPER            (0x00000000)
#define VAL_GPIOF_OSPEEDR           (0x00000000)
#define VAL_GPIOF_PUPDR             (0x00000000)
#define VAL_GPIOF_ODR               (0x00000000)
#define VAL_GPIOF_AFRL              (0x00000000)
#define VAL_GPIOF_AFRH              (0x00000000)

/*
 * GPIOG setup:
 * No GPIOG on STM32F4 LQFP64
 */
#define VAL_GPIOG_MODER             (0x00000000)
#define VAL_GPIOG_OTYPER            (0x00000000)
#define VAL_GPIOG_OSPEEDR           (0x00000000)
#define VAL_GPIOG_PUPDR             (0x00000000)
#define VAL_GPIOG_ODR               (0x00000000)
#define VAL_GPIOG_AFRL              (0x00000000)
#define VAL_GPIOG_AFRH              (0x00000000)

/*
 * GPIOH setup:
 *
 * PH0  - OSC_IN                    (input floating).
 * PH1  - OSC_OUT                   (input floating).
 */
#define VAL_GPIOH_MODER             (PIN_MODE_INPUT(GPIOH_OSC_IN) |         \
                                     PIN_MODE_INPUT(GPIOH_OSC_OUT))
#define VAL_GPIOH_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOH_OSC_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOH_OSC_OUT))
#define VAL_GPIOH_OSPEEDR           (PIN_OSPEED_100M(GPIOH_OSC_IN) |        \
                                     PIN_OSPEED_100M(GPIOH_OSC_OUT))
#define VAL_GPIOH_PUPDR             (PIN_PUDR_FLOATING(GPIOH_OSC_IN) |     \
                                     PIN_PUDR_FLOATING(GPIOH_OSC_OUT))
#define VAL_GPIOH_ODR               (PIN_ODR_HIGH(GPIOH_OSC_IN) |           \
                                     PIN_ODR_HIGH(GPIOH_OSC_OUT))
#define VAL_GPIOH_AFRL              (PIN_AFIO_AF(GPIOH_OSC_IN, 0) |         \
                                     PIN_AFIO_AF(GPIOH_OSC_OUT, 0))
#define VAL_GPIOH_AFRH              (0)

/*
 * GPIOI setup:
 * No GPIOI on STM32F4 LQFP64
 */
#define VAL_GPIOI_MODER             (0x00000000)
#define VAL_GPIOI_OTYPER            (0x00000000)
#define VAL_GPIOI_OSPEEDR           (0x00000000)
#define VAL_GPIOI_PUPDR             (0x00000000)
#define VAL_GPIOI_ODR               (0x00000000)
#define VAL_GPIOI_AFRL              (0x00000000)
#define VAL_GPIOI_AFRH              (0x00000000)
 
#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
