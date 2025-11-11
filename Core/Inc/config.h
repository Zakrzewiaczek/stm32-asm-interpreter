

/**
 * @file    config.h
 * @brief   Global configuration parameters for ARM interpreter
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Centralized configuration file containing all tunable parameters
 * for the ARM assembly interpreter. Modify these values to adjust
 * memory usage, parser limits, and system behavior.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ===================================================================
 *  Memory Configuration
 * =================================================================== */

/** @defgroup Memory_Config Memory System Configuration
 * @brief Memory layout and allocation limits
 * @{
 */

/**
 * @brief Stack size in bytes (2KB default)
 *
 * Reserved stack region size. Stack grows downward from RAM_END.
 * Must be multiple of 4 bytes for ARM alignment requirements.
 */
#define STACK_SIZE 0x800UL

/** @} */

/* ===================================================================
 *  Parser Configuration
 * =================================================================== */

/** @defgroup Parser_Config Parser and Instruction Limits
 * @brief Limits for instruction parsing and operand handling
 * @{
 */

/**
 * @brief Maximum operands per instruction
 *
 * Most ARM instructions have 2-3 operands, but some complex
 * instructions may require more.
 */
#define MAX_OPERANDS 4

/**
 * @brief Maximum instruction line length in characters
 *
 * Includes mnemonic, operands, and whitespace. Lines longer
 * than this will be truncated during parsing.
 */
#define MAX_INSTRUCTION_LENGTH 128

/**
 * @brief Maximum instruction mnemonic length
 *
 * Longest ARM mnemonics are typically 4-8 characters (e.g., "STRH").
 */
#define MAX_MNEMONIC_LENGTH 16

/**
 * @brief Maximum label name length
 *
 * For future label/symbol support in branch instructions.
 */
#define MAX_LABEL_LENGTH 32

/** @} */

/* ===================================================================
 *  Logging Configuration
 * =================================================================== */

/** @defgroup Logging_Config Logging System Configuration
 * @brief Default logging behavior settings
 * @{
 */

/**
 * @brief Default log level at startup
 *
 * Can be changed at runtime via .log command.
 */
#define DEFAULT_LOG_LEVEL 4

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
