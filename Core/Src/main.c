/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : STM32 ARM Assembly Interpreter - Main Program
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 Jakub Zakrzewski.
 * Licensed under MIT License - see LICENSE file for details.
 *
 * This is an interactive ARM assembly language interpreter that runs on
 * STM32L476RG microcontroller. It provides a REPL (Read-Eval-Print Loop)
 * environment for executing ARM assembly instructions via UART.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "parser.h"
#include "commands.h"
#include "errors.h"
#include "cpu.h"
#include "memory.h"
#include "log.h"
#include "debug_commands.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Redirect stdout to UART for printf support
 * @param ch Character to send
 * @return Character sent
 */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/* USER CODE END 0 */

/**
 * @brief  Application entry point and main REPL loop
 *
 * Initializes hardware, displays welcome banner, and enters infinite
 * loop accepting assembly instructions via UART. Each instruction is
 * parsed, validated, and executed with error handling.
 *
 * @retval int (never returns)
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  uint8_t rx_data[32];
  uint8_t i = 0;
  uint8_t ch;

  // Example: Configure GPIO for LED (with new safe memory access)
  // Enable GPIOA clock
  mem_write32((uint32_t)get_register("RCC_AHB2ENR")->address, (1 << 0));

  // Configure PA5 as output
  uint32_t moder_val;
  if (is_ok(mem_read32((uint32_t)get_register("GPIOA_MODER")->address, &moder_val)))
  {
    moder_val = (moder_val & ~(0x3 << (5 * 2))) | (0x1 << (5 * 2));
    mem_write32((uint32_t)get_register("GPIOA_MODER")->address, moder_val);
  }

  // Toggle LED
  uint32_t odr_val;
  if (is_ok(mem_read32((uint32_t)get_register("GPIOA_ODR")->address, &odr_val)))
  {
    mem_write32((uint32_t)get_register("GPIOA_ODR")->address, odr_val ^ (1 << 5));
  }

  printf("RAM start addr:         %p\n", (void *)RAM_START);
  printf("RAM end addr:           %p\n", (void *)(RAM_END - 1));
  printf("RAM size:               %lu bytes (0x%lX)\n", (uint32_t)RAM_SIZE, (uint32_t)RAM_SIZE);
  printf("PERIPH_BASE start addr: %p\n", (void *)PERIPH_BASE);

  // memset((void *)RAM_START, 0, RAM_SIZE); // Clear RAM memory

  set_log_level(LOG_LEVEL_DEBUG);

  while (1)
  {
    if (HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY) == HAL_OK)
    {
      if (ch == '\\') // Continuation character
      {
        rx_data[i++] = ' '; // Replace with space
        continue;
      }

      if (ch == '\n' || i >= sizeof(rx_data) - 1)
      {
        rx_data[i] = '\0';

        i = 0;

        // Check if it's a debug command (starts with '.')
        if (is_debug_command((const char *)rx_data))
        {
          result_t res = execute_debug_command((const char *)rx_data);
          if (is_error(res))
          {
            printf("❌ Debug command failed (code=%d)\r\n", res.code);
          }
          continue; // Skip normal instruction processing
        }

        char mnemonic[16];
        operand_t operands[3];
        uint8_t operand_count = 0;

        // Parse instruction
        result_t res = parse_instruction((const char *)rx_data, mnemonic, operands, &operand_count);
        if (is_error(res))
        {
          continue;
        }

        // Validate instruction
        res = validate_instruction(mnemonic, operands, operand_count);
        if (is_error(res))
        {
          continue;
        }

        // Execute instruction
        (void)execute_instruction(mnemonic, operands, operand_count);
      }
      else
      {
        rx_data[i++] = ch;
      }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
