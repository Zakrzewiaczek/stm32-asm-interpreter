/**
 * @file parser.h
 * @brief ARM assembly instruction parser
 *
 * Parses ARM assembly syntax into structured operand representations.
 * Supports registers, immediates, labels, and various memory addressing modes.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Operand type enumeration
     *
     * Each type is a distinct bit flag, allowing bitmask operations for
     * instruction operand pattern matching.
     */
    typedef enum
    {
        OPERAND_NONE = 0x00,      /**< No operand */
        OPERAND_REGISTER = 0x01,  /**< Register operand (R0-R15) */
        OPERAND_IMMEDIATE = 0x02, /**< Immediate value (#123, #0x1A, etc.) */

        // Memory addressing modes (separate bits for each mode)
        OPERAND_MEM_SIMPLE = 0x04,     /**< Simple memory: [Rn] */
        OPERAND_MEM_OFFSET = 0x08,     /**< Memory with offset: [Rn, #imm] */
        OPERAND_MEM_REG_OFFSET = 0x10, /**< Memory with register offset: [Rn, Rm] */
        OPERAND_MEM_PRE_INC = 0x20,    /**< Pre-indexed with writeback: [Rn, #imm]! */
        OPERAND_MEM_POST_INC = 0x40,   /**< Post-indexed: [Rn], #imm */

        OPERAND_LABEL = 0x80 /**< Label (for branches, etc.) */
    } operand_type_t;

    /** @defgroup Parser_Type_Shortcuts Operand Type Shortcuts
     * @brief Convenient short names for operand types
     * @{
     */

#define OP_REG OPERAND_REGISTER  /**< Register shortcut */
#define OP_IMM OPERAND_IMMEDIATE /**< Immediate shortcut */
#define OP_LABEL OPERAND_LABEL   /**< Label shortcut */

    /** @} */

    /** @defgroup Parser_Memory_Shortcuts Memory Addressing Mode Shortcuts
     * @brief Convenient short names for memory addressing modes
     * @{
     */

#define OP_MEM_SIMPLE OPERAND_MEM_SIMPLE         /**< Simple memory [Rn] */
#define OP_MEM_OFFSET OPERAND_MEM_OFFSET         /**< Offset memory [Rn, #imm] */
#define OP_MEM_REG_OFFSET OPERAND_MEM_REG_OFFSET /**< Register offset [Rn, Rm] */
#define OP_MEM_PRE OPERAND_MEM_PRE_INC           /**< Pre-indexed [Rn, #imm]! */
#define OP_MEM_POST OPERAND_MEM_POST_INC         /**< Post-indexed [Rn], #imm */

/** @} */

/** @defgroup Parser_Memory_Combinations Memory Addressing Combinations
 * @brief Combined memory addressing mode bitmasks
 * @{
 */

/** Basic memory modes (simple, offset, register offset) */
#define OP_MEM_BASIC (OP_MEM_SIMPLE | OP_MEM_OFFSET | OP_MEM_REG_OFFSET)

/** All memory addressing modes */
#define OP_MEM_ALL (OP_MEM_SIMPLE | OP_MEM_OFFSET | OP_MEM_REG_OFFSET | OP_MEM_PRE | OP_MEM_POST)

/** Legacy compatibility - any memory mode */
#define OP_MEM OP_MEM_ALL

    /** @} */

    /**
     * @brief Parsed operand structure
     *
     * Represents a single operand parsed from assembly instruction.
     * The type field determines which union member is valid.
     */
    typedef struct
    {
        operand_type_t type; /**< Type of operand */

        union
        {
            uint8_t reg;        /**< Register number (R0-R15) for OPERAND_REGISTER */
            uint32_t immediate; /**< Immediate value for OPERAND_IMMEDIATE */

            /**
             * @brief Memory operand details
             *
             * Valid for all OPERAND_MEM_* types. The specific fields used depend
             * on the addressing mode:
             * - OPERAND_MEM_SIMPLE: only base_reg
             * - OPERAND_MEM_OFFSET: base_reg + offset
             * - OPERAND_MEM_REG_OFFSET: base_reg + offset_reg
             * - OPERAND_MEM_PRE_INC: base_reg + offset, writeback=true
             * - OPERAND_MEM_POST_INC: base_reg, offset, writeback=true
             */
            struct
            {
                uint8_t base_reg;   /**< Base register number */
                uint32_t offset;    /**< Immediate offset value (signed, stored as uint32_t) */
                uint8_t offset_reg; /**< Offset register number (0xFF = not used) */
                bool writeback;     /**< Writeback flag for pre/post-indexed modes */
            } memory;

            char label[32]; /**< Label name for OPERAND_LABEL (null-terminated) */
        } value;
    } operand_t;

    /**
     * @brief Parse instruction string into mnemonic and operands
     *
     * Parses ARM assembly instruction syntax. The input string is analyzed to extract:
     * - Instruction mnemonic (e.g., "mov", "ldr", "add")
     * - Operands (registers, immediates, memory operands, labels)
     *
     * Examples:
     * - "mov r0, #123" -> mnemonic="mov", 2 operands (register, immediate)
     * - "ldr r1, [r2, #4]" -> mnemonic="ldr", 2 operands (register, memory)
     * - "str r3, [r4], #8" -> mnemonic="str", 2 operands (register, post-indexed memory)
     *
     * @param input Input string containing instruction (null-terminated)
     * @param mnemonic Output buffer for mnemonic (must be at least 16 bytes)
     * @param operands Output array for parsed operands (must have at least 3 elements)
     * @param operand_count Output parameter for number of operands parsed
     * @return Result with ERR_OK on success, error code with context on failure
     */
    result_t parse_instruction(const char *input, char *mnemonic, operand_t *operands, uint8_t *operand_count);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
