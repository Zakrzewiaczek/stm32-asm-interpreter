/**
 * @file commands.h
 * @brief Instruction validation and execution interface
 *
 * Provides instruction descriptor table and validation/execution functions
 * for ARM assembly instructions. Each instruction is described by allowed
 * operand patterns and has associated validation and execution handlers.
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
     * @brief Operand type mask (alias for operand_type_t)
     *
     * Used in instruction descriptors to specify allowed operand types.
     */
    typedef operand_type_t operand_mask_t;

    /**
     * @brief Instruction descriptor structure
     *
     * Describes an ARM instruction including its mnemonic, operand requirements,
     * and function pointers for validation and execution.
     */
    typedef struct
    {
        const char *mnemonic;                                      /**< Instruction mnemonic (e.g., "mov", "ldr") */
        uint8_t min_operands;                                      /**< Minimum number of operands required */
        uint8_t max_operands;                                      /**< Maximum number of operands allowed */
        uint16_t operand_patterns[4];                              /**< Bitmasks of allowed types for each operand position */
        result_t (*validate)(const operand_t *ops, uint8_t count); /**< Optional custom validation function (can be NULL) */
        result_t (*execute)(const operand_t *ops, uint8_t count);  /**< Instruction execution function */
    } instruction_descriptor_t;

    /**
     * @brief Global instruction descriptor table
     *
     * Array of all supported instructions. Defined in commands.c.
     * Terminated by entry with NULL mnemonic.
     */
    extern const instruction_descriptor_t instructions[];

    /**
     * @brief Validate instruction mnemonic and operands
     *
     * Checks if the instruction exists, has correct number of operands,
     * and each operand type matches the allowed patterns. Optionally runs
     * custom validation if defined for the instruction.
     *
     * @param mnemonic Instruction mnemonic (e.g., "mov", "ldr")
     * @param operands Array of parsed operands
     * @param operand_count Number of operands
     * @return Result with ERR_OK on success, error code with context on failure
     */
    result_t validate_instruction(const char *mnemonic, const operand_t *operands, uint8_t operand_count);

    /**
     * @brief Execute validated instruction
     *
     * Executes the instruction by calling its execute function pointer.
     * Assumes instruction has already been validated.
     *
     * @param mnemonic Instruction mnemonic
     * @param operands Array of operands
     * @param operand_count Number of operands
     * @return Result with ERR_OK on success, error code with context on failure
     */
    result_t execute_instruction(const char *mnemonic, const operand_t *operands, uint8_t operand_count);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_H */
