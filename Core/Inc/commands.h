/**
 * @file    commands.h
 * @brief   ARM instruction command dispatcher and validation
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * This module provides the instruction lookup table and dispatch mechanism
 * for validating and executing ARM assembly instructions. Each instruction
 * is described by an instruction_descriptor_t structure containing operand
 * patterns, validation rules, and execution handlers.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "parser.h"
#include "errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Operand type mask for pattern matching
     */
    typedef operand_type_t operand_mask_t;

    /**
     * @brief Instruction descriptor structure
     *
     * Defines all metadata for a single ARM instruction including operand
     * requirements, validation rules, and execution handler.
     */
    typedef struct
    {
        const char *mnemonic;                                      ///< Instruction mnemonic (e.g., "MOV", "ADD")
        uint8_t min_operands;                                      ///< Minimum number of operands required
        uint8_t max_operands;                                      ///< Maximum number of operands allowed
        uint16_t operand_patterns[4];                              ///< Valid operand type patterns per position
        result_t (*validate)(const operand_t *ops, uint8_t count); ///< Custom validation function (optional)
        result_t (*execute)(const operand_t *ops, uint8_t count);  ///< Instruction execution handler
    } instruction_descriptor_t;

    /**
     * @brief Global instruction lookup table
     *
     * NULL-terminated array of all supported instructions. Used by
     * validate_instruction() and execute_instruction() for dispatch.
     */
    extern const instruction_descriptor_t instructions[];

    /**
     * @brief Validate instruction operands against instruction descriptor
     *
     * Looks up the instruction by mnemonic and validates that:
     * - Operand count is within min/max bounds
     * - Each operand type matches the allowed pattern
     * - Custom validation (if provided) passes
     *
     * @param[in] mnemonic       Instruction mnemonic string (e.g., "MOV")
     * @param[in] operands       Array of parsed operands
     * @param[in] operand_count  Number of operands in array
     *
     * @retval OK                     Validation successful
     * @retval ERR_VALIDATE_UNKNOWN_INSTRUCTION  Instruction not found
     * @retval ERR_VALIDATE_TOO_FEW_OPERANDS     Too few operands
     * @retval ERR_VALIDATE_TOO_MANY_OPERANDS    Too many operands
     * @retval ERR_VALIDATE_INVALID_OPERAND_TYPE Invalid operand type
     * @retval ERR_VALIDATE_CUSTOM_FAILED        Custom validation failed
     */
    result_t validate_instruction(const char *mnemonic, const operand_t *operands, uint8_t operand_count);

    /**
     * @brief Execute validated instruction
     *
     * Dispatches to the appropriate instruction handler. Operands must
     * be pre-validated using validate_instruction().
     *
     * @param[in] mnemonic       Instruction mnemonic string
     * @param[in] operands       Array of validated operands
     * @param[in] operand_count  Number of operands
     *
     * @retval OK                                Execution successful
     * @retval ERR_VALIDATE_UNKNOWN_INSTRUCTION  Instruction not found
     * @retval ERR_EXEC_*                        Execution error (see errors.h)
     *
     * @note Assumes operands have been validated beforehand
     */
    result_t execute_instruction(const char *mnemonic, const operand_t *operands, uint8_t operand_count);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_H */
