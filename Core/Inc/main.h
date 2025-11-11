/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32l4xx_hal.h"

#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "log.h"
#include "parser.h"
#include "commands.h"
#include "errors.h"

#include "stm32l476xx_registers.h"

  void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
