

/**
 * @file    data_transfer.h
 * @brief   ARM data transfer instruction implementations
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Implementation of ARM data transfer instructions including PUSH, POP, MOV, LDR,
 * and STR variants with support for all ARM addressing modes
 */

#ifndef INSTR_DATA_TRANSFER_H
#define INSTR_DATA_TRANSFER_H

#include "parser.h"
#include "errors.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief MOV - Move data to register
     *
     * Transfers data from register or immediate value to destination register.
     * Does not update condition flags.
     *
     * Syntax:
     * - MOV Rd, Rm        (register to register)
     * - MOV Rd, #imm      (immediate to register)
     *
     * @param[in] operands       Array of operands [dest_reg, source]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK                         Move successful
     * @retval ERR_EXEC_INVALID_OPERAND_TYPE  Invalid operand type
     */
    result_t instr_mov(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief MOVS - Move data with condition flags update
     *
     * Same as MOV but updates N (negative) and Z (zero) flags based
     * on the result value.
     *
     * @param[in] operands       Array of operands [dest_reg, source]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK                         Move successful, flags updated
     * @retval ERR_EXEC_INVALID_OPERAND_TYPE  Invalid operand type
     */
    result_t instr_movs(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief LDRB - Load byte from memory to register
     *
     * Loads an 8-bit value from memory and zero-extends to 32 bits.
     * Supports all ARM addressing modes.
     *
     * @param[in] operands       Array of operands [dest_reg, memory_operand]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK       Load successful
     * @retval ERR_MEM_*  Memory access error
     */
    result_t instr_ldrb(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief LDRH - Load halfword from memory to register
     *
     * Loads a 16-bit value from memory and zero-extends to 32 bits.
     * Memory address must be 2-byte aligned.
     *
     * @param[in] operands       Array of operands [dest_reg, memory_operand]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK                        Load successful
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 2-byte aligned
     * @retval ERR_MEM_*                 Memory access error
     */
    result_t instr_ldrh(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief LDR - Load word from memory to register
     *
     * Loads a 32-bit value from memory to register.
     * Memory address must be 4-byte aligned.
     *
     * @param[in] operands       Array of operands [dest_reg, memory_operand]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK                        Load successful
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 4-byte aligned
     * @retval ERR_MEM_*                 Memory access error
     */
    result_t instr_ldr(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief STRB - Store byte from register to memory
     *
     * Stores the lower 8 bits of a register to memory.
     * Upper 24 bits are ignored.
     *
     * @param[in] operands       Array of operands [source_reg, memory_operand]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK       Store successful
     * @retval ERR_MEM_*  Memory access error
     */
    result_t instr_strb(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief STRH - Store halfword from register to memory
     *
     * Stores the lower 16 bits of a register to memory.
     * Upper 16 bits are ignored. Memory address must be 2-byte aligned.
     *
     * @param[in] operands       Array of operands [source_reg, memory_operand]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK                        Store successful
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 2-byte aligned
     * @retval ERR_MEM_*                 Memory access error
     */
    result_t instr_strh(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief STR - Store word from register to memory
     *
     * Stores a complete 32-bit register value to memory.
     * Memory address must be 4-byte aligned.
     *
     * @param[in] operands       Array of operands [source_reg, memory_operand]
     * @param[in] operand_count  Number of operands (must be 2)
     *
     * @retval OK                        Store successful
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 4-byte aligned
     * @retval ERR_MEM_*                 Memory access error
     */
    result_t instr_str(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief PUSH - Push register onto stack
     *
     * Pushes a 32-bit register value onto the stack using ARM Full Descending
     * convention (pre-decrement). Decrements SP by 4, then stores the value.
     *
     * Syntax:
     * - PUSH Rn
     *
     * @param[in] operands       Array of operands [source_reg]
     * @param[in] operand_count  Number of operands (must be 1)
     *
     * @retval OK                   Push successful
     * @retval ERR_STACK_OVERFLOW   Stack full
     * @retval ERR_MEM_*            Memory write error
     *
     * @note Updates SP register (R13)
     */
    result_t instr_push(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief POP - Pop value from stack to register
     *
     * Pops a 32-bit value from the stack into a register using ARM Full
     * Descending convention. Loads the value, then increments SP by 4.
     *
     * Syntax:
     * - POP Rd
     *
     * @param[in] operands       Array of operands [dest_reg]
     * @param[in] operand_count  Number of operands (must be 1)
     *
     * @retval OK                    Pop successful
     * @retval ERR_STACK_UNDERFLOW   Stack empty
     * @retval ERR_MEM_*             Memory read error
     *
     * @note Updates SP register (R13)
     */
    result_t instr_pop(const operand_t *operands, uint8_t operand_count);

#ifdef __cplusplus
}
#endif

#endif /* INSTR_DATA_TRANSFER_H */
