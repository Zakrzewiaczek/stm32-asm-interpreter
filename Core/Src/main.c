/**
 * @file    main.c
 * @brief   STM32L476RG ARM Assembly Interpreter Main Application
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Main application entry point for the ARM assembly interpreter.
 * Provides UART-based REPL interface for executing ARM assembly
 * instructions and debug commands interactively.
 *
 * Features:
 * - Interactive REPL via UART (115200 baud)
 * - ARM assembly instruction parsing and execution
 * - Debug command system (commands starting with '.')
 * - Memory-mapped peripheral access with safety validation
 * - Stack management with ARM FD convention
 * - Comprehensive error handling and logging
 */

#include "main.h"

#include "parser.h"
#include "commands.h"
#include "errors.h"
#include "cpu.h"
#include "memory.h"
#include "stack.h"
#include "log.h"
#include "debug_commands.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    uint8_t rx_data[MAX_INSTRUCTION_LENGTH];
    uint8_t i = 0;
    uint8_t ch;

    // Example: Configure GPIO for LED (with new safe memory access)
    // Enable GPIOA clock
    // TODO: correct all comments to english
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

    // Print logo
    printf("┏━┓╺┳╸┏┳┓┏━┓┏━┓   ┏━┓┏━┓┏━┓┏━╸┏┳┓┏┓ ╻  ┏━╸┏━┓\n┗━┓ ┃ ┃┃┃╺━┫┏━┛   ┣━┫┗━┓┗━┓┣╸ ┃┃┃┣┻┓┃  ┣╸ ┣┳┛\n┗━┛ ╹ ╹ "
           "╹┗━┛┗━╸   ╹ ╹┗━┛┗━┛┗━╸╹ ╹┗━┛┗━╸┗━╸╹┗╸\n\n");

    memory_init();
    stack_init();
    registers_init();
    cmd_mem(NULL); // Display memory informations

    set_log_level(LOG_LEVEL_DEBUG);

    /* ===================================================================
     *  Main REPL Loop
     * =================================================================== */

    while (1)
    {
        if (HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            // Handle line continuation character
            if (ch == '\\')
            {
                rx_data[i++] = ' '; // Replace with space for parsing
                continue;
            }

            // Process complete command on newline or buffer full
            if (ch == '\n' || i >= sizeof(rx_data) - 1)
            {
                rx_data[i] = '\0'; // Null-terminate command string
                i = 0;             // Reset buffer index

                if (is_debug_command((const char *)rx_data))
                {
                    result_t res = execute_debug_command((const char *)rx_data);
                    if (is_error(res))
                    {
                        printf("❌ Debug command failed (code=%d)\n", res.code);
                    }
                    continue; // Skip instruction processing
                }

                char mnemonic[16];
                operand_t operands[MAX_OPERANDS];
                uint8_t operand_count = 0;

                // Parse instruction syntax
                result_t res = parse_instruction((const char *)rx_data, mnemonic, operands, &operand_count);
                if (is_error(res))
                {
                    continue; // Error already logged by parser
                }

                // Validate instruction and operand compatibility
                res = validate_instruction(mnemonic, operands, operand_count);
                if (is_error(res))
                {
                    continue; // Error already logged by validator
                }

                // Execute instruction
                (void)execute_instruction(mnemonic, operands, operand_count);
            }
            else
            {
                // Accumulate characters in input buffer
                rx_data[i++] = ch;
            }
        }
    }
}

/**
 * @brief System Clock Configuration
 *
 * Configures the system clock to use MSI (Multi-Speed Internal) oscillator
 * at 4 MHz with no PLL. This provides a stable, low-power clock source
 * suitable for the ARM assembly interpreter application.
 *
 * Clock Configuration:
 * - MSI Range 6: 4 MHz
 * - SYSCLK: 4 MHz (MSI direct)
 * - HCLK: 4 MHz (no division)
 * - PCLK1: 4 MHz (no division)
 * - PCLK2: 4 MHz (no division)
 * - Flash Latency: 0 wait states
 *
 * @note This function is generated by STM32CubeMX
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure voltage scaling for performance optimization
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure MSI oscillator as main clock source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6; // 4 MHz
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;    // PLL disabled
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure system and peripheral clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI; // MSI as SYSCLK
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;     // HCLK = SYSCLK
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;      // PCLK1 = HCLK
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;      // PCLK2 = HCLK

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief USART2 Initialization
 *
 * Configures USART2 for console communication with the ARM assembly
 * interpreter. USART2 is connected to ST-LINK virtual COM port on
 * Nucleo-L476RG board.
 *
 * UART Configuration:
 * - Baud Rate: 115200 bps
 * - Data Bits: 8
 * - Stop Bits: 1
 * - Parity: None
 * - Flow Control: None
 * - Mode: TX/RX
 *
 * @note This function is generated by STM32CubeMX
 */
static void MX_USART2_UART_Init(void)
{
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
}

/**
 * @brief GPIO Initialization
 *
 * Enables GPIO peripheral clocks required by the application.
 * GPIOA clock is enabled for LED control and USART2 pins.
 *
 * @note This function is generated by STM32CubeMX
 */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

/**
 * @brief Error Handler
 *
 * This function is executed in case of error occurrence.
 * Disables interrupts and enters infinite loop for debugging.
 *
 * @note User can add their own implementation to report the error
 *       back to the main program
 */
void Error_Handler(void)
{
    /* Disable interrupts */
    __disable_irq();

    /* Infinite loop for error state */
    while (1)
    {
        /* Stay here for debugging */
    }
}
#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
    /* You can add your own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
