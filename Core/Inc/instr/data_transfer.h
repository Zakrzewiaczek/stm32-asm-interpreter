/**
 * @file data_transfer.h
 * @brief Data transfer instructions (MOV, LDR, STR, PUSH, POP)
 *
 * Implements ARM data transfer instructions including:
 * - MOV/MOVS: Move data between registers or from immediate
 * - LDR/LDRB/LDRH: Load from memory to register (32/8/16-bit)
 * - STR/STRB/STRH: Store from register to memory (32/8/16-bit)
 * - PUSH/POP: Stack operations
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
     * @brief MOV instruction - Move data to register
     *
     * Syntax:
     * - MOV Rd, Rm       (register to register)
     * - MOV Rd, #imm     (immediate to register)
     *
     * @param operands Array of operands (dest_reg, source)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_mov(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief MOVS instruction - Move with flags update
     *
     * Same as MOV but updates N (negative) and Z (zero) flags based on result.
     *
     * @param operands Array of operands (dest_reg, source)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_movs(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief LDRB instruction - Load byte from memory
     *
     * Loads 8-bit value from memory into register (zero-extended to 32-bit).
     * Supports all addressing modes.
     *
     * @param operands Array of operands (dest_reg, memory_operand)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_ldrb(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief LDRH instruction - Load halfword from memory
     *
     * Loads 16-bit value from memory into register (zero-extended to 32-bit).
     * Requires 2-byte aligned address. Supports all addressing modes.
     *
     * @param operands Array of operands (dest_reg, memory_operand)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_ldrh(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief LDR instruction - Load word from memory
     *
     * Loads 32-bit value from memory into register.
     * Requires 4-byte aligned address.
     *
     * Supported addressing modes:
     * - LDR Rd, [Rn]           (simple)
     * - LDR Rd, [Rn, #offset]  (offset)
     * - LDR Rd, [Rn, Rm]       (register offset)
     * - LDR Rd, [Rn, #offset]! (pre-indexed with writeback)
     * - LDR Rd, [Rn], #offset  (post-indexed with writeback)
     *
     * @param operands Array of operands (dest_reg, memory_operand)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_ldr(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief STRB instruction - Store byte to memory
     *
     * Stores lower 8 bits of register to memory.
     * Supports all addressing modes.
     *
     * @param operands Array of operands (source_reg, memory_operand)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_strb(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief STRH instruction - Store halfword to memory
     *
     * Stores lower 16 bits of register to memory.
     * Requires 2-byte aligned address. Supports all addressing modes.
     *
     * @param operands Array of operands (source_reg, memory_operand)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_strh(const operand_t *operands, uint8_t operand_count);

    /**
     * @brief STR instruction - Store word to memory
     *
     * Stores 32-bit register value to memory.
     * Requires 4-byte aligned address. Supports all addressing modes.
     *
     * @param operands Array of operands (source_reg, memory_operand)
     * @param operand_count Number of operands (must be 2)
     * @return Result with ERR_OK on success, error code otherwise
     */
    result_t instr_str(const operand_t *operands, uint8_t operand_count);

#ifdef __cplusplus
}
#endif

#endif /* INSTR_DATA_TRANSFER_H */
