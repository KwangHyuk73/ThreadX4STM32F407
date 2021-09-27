/* USER CODE BEGIN Header */

/**
 ******************************************************************************
 *	File		:	main.h
 *	Brief		:	Header file of ThreadX demo application.
 *	Created on	:	Sep 17, 2021
 *	Author		:	William, An.
 *	Email		:	ponytail2k@gmail.com
 ******************************************************************************
 *
 * Copyright (c) 2021 Lee & An Partners Co., Ltd. All rights reserved.
 *
 * This software component is licensed by Lee & An Partners under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os.h"

#include "kprintf.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern void _tx_timer_interrupt(void);
extern void __tx_PendSVHandler(void);

extern void (*TST_IRQHandler) (void);
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OrangeLED_Pin GPIO_PIN_12
#define OrangeLED_GPIO_Port GPIOD
#define GreenLED_Pin GPIO_PIN_13
#define GreenLED_GPIO_Port GPIOD
#define IntGen_Pin GPIO_PIN_6
#define IntGen_GPIO_Port GPIOC
#define EINT7_Pin GPIO_PIN_7
#define EINT7_GPIO_Port GPIOC
#define EINT7_EXTI_IRQn EXTI9_5_IRQn
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
