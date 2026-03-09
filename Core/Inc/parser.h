

/**
 * @file    parser.h
 * @brief   ARM assembly instruction parser
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Parses ARM assembly instruction strings into structured operand
 * representations. Supports all standard ARM operand types including
 * registers, immediates, and memory addressing modes with pre/post
 * indexing and writeback.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "errors.h"
#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Operand type flags (bitfield)
     *
     * Each operand type is represented by a unique bit position to allow
     * bitwise combination for pattern matching in instruction descriptors.
     */
    typedef enum
    {
        OPERAND_NONE = 0x00,      ///< No operand (placeholder)
        OPERAND_REGISTER = 0x01,  ///< Register operand (R0-R15)
        OPERAND_IMMEDIATE = 0x02, ///< Immediate value (#123)

        /* Memory Addressing Modes ----------------------------------- */
        OPERAND_MEM_SIMPLE = 0x04,     ///< Simple memory:   [Rn]
        OPERAND_MEM_OFFSET = 0x08,     ///< Offset memory:   [Rn, #offset]
        OPERAND_MEM_REG_OFFSET = 0x10, ///< Register offset: [Rn, Rm]
        OPERAND_MEM_PRE_INC = 0x20,    ///< Pre-increment:   [Rn, #offset]!
        OPERAND_MEM_POST_INC = 0x40,   ///< Post-increment:  [Rn], #offset

        /**
         * @brief Memory operand by symbolic register name: [RCC_MODER]
         *
         * Allows addressing using a named register or peripheral symbol
         * defined in stm32l476xx_registers.h/.c. The symbol is stored in
         * operand.value.label and resolved at execution time.
         */
        OPERAND_MEM_SYMBOL = 0x80,

        /**
         * @brief Generic label/symbol reference (non-memory)
         *
         * Reserved for future use (branch targets, literals, etc.).
         */
        OPERAND_LABEL = 0x100,

        /**
         * @brief Register list operand ({R0-R3, LR})
         *
         * Used by PUSH/POP and LDM/STM instructions. Stored as a
         * 16-bit bitmask where bit N represents register RN.
         */
        OPERAND_REG_LIST = 0x200
    } operand_type_t;

    /** @defgroup Operand_Aliases Short Operand Type Names
     * @brief Convenient aliases for operand types
     * @{
     */

#define OP_REG      OPERAND_REGISTER  ///< Register operand alias
#define OP_IMM      OPERAND_IMMEDIATE ///< Immediate operand alias
#define OP_LABEL    OPERAND_LABEL     ///< Label operand alias
#define OP_REG_LIST OPERAND_REG_LIST  ///< Register list operand alias

    /** @} */

    /** @defgroup Memory_Aliases Memory Addressing Mode Aliases
     * @brief Convenient aliases for memory operand patterns
     * @{
     */

#define OP_MEM_SIMPLE     OPERAND_MEM_SIMPLE     ///< [Rn] alias
#define OP_MEM_OFFSET     OPERAND_MEM_OFFSET     ///< [Rn, #offset] alias
#define OP_MEM_REG_OFFSET OPERAND_MEM_REG_OFFSET ///< [Rn, Rm] alias
#define OP_MEM_PRE        OPERAND_MEM_PRE_INC    ///< [Rn, #offset]! alias
#define OP_MEM_POST       OPERAND_MEM_POST_INC   ///< [Rn], #offset alias
#define OP_MEM_SYM        OPERAND_MEM_SYMBOL     ///< [SYMBOL] alias

/** @} */

/** @defgroup Memory_Groups Memory Operand Pattern Groups
 * @brief Bitwise combinations for pattern matching
 * @{
 */

/**
 * @brief Basic memory addressing modes (no writeback)
 */
#define OP_MEM_BASIC (OP_MEM_SIMPLE | OP_MEM_OFFSET | OP_MEM_REG_OFFSET)

/**
 * @brief All memory addressing modes
 */
#define OP_MEM_ALL (OP_MEM_SIMPLE | OP_MEM_OFFSET | OP_MEM_REG_OFFSET | OP_MEM_PRE | OP_MEM_POST | OP_MEM_SYM)

/**
 * @brief Generic memory operand (all modes)
 */
#define OP_MEM OP_MEM_ALL

    /** @} */

    /**
     * @brief Parsed operand representation
     *
     * Contains the operand type and value in a discriminated union.
     * The type field determines which union member is valid.
     */
    typedef struct
    {
        operand_type_t type; ///< Operand type (determines union member)

        union
        {
            uint8_t reg;       ///< Register number (0-15) for OPERAND_REGISTER
            int32_t immediate; ///< Immediate value for OPERAND_IMMEDIATE (signed)
            uint16_t reg_list; ///< Register bitmask for OPERAND_REG_LIST (bit N = RN)

            /**
             * @brief Memory operand details
             *
             * Used for all memory addressing modes. The specific mode is
             * determined by the type field.
             */
            struct
            {
                uint8_t base_reg;   ///< Base register (0-15)
                int32_t offset;     ///< Offset value (signed, for offset modes)
                uint8_t offset_reg; ///< Offset register (for reg offset mode)
                bool writeback;     ///< Writeback flag (pre/post increment)
            } memory;

            char label[32]; ///< Label name for OPERAND_LABEL (future)
        } value;
    } operand_t;

    /**
     * @brief Parse ARM assembly instruction string
     *
     * Parses a complete instruction line into mnemonic and operands.
     * Handles all ARM operand types and addressing modes with full
     * syntax validation.
     *
     * @param[in]  input          Instruction string (e.g., "MOV R0, #42")
     * @param[out] mnemonic       Buffer for instruction mnemonic (min MAX_MNEMONIC_LENGTH)
     * @param[out] operands       Array for parsed operands (min MAX_OPERANDS)
     * @param[out] operand_count  Number of operands parsed
     *
     * @retval OK                         Parsing successful
     * @retval ERR_NULL_POINTER           Invalid input or output pointers
     * @retval ERR_PARSE_EMPTY_MNEMONIC   No instruction mnemonic found
     * @retval ERR_PARSE_INVALID_OPERAND  Malformed operand syntax
     * @retval ERR_PARSE_TOO_MANY_OPERANDS  More than MAX_OPERANDS operands
     * @retval ERR_PARSE_*               Other parsing errors (see errors.h)
     *
     * @note Output buffers must be properly sized according to config.h limits
     */
    result_t parse_instruction(const char *input, char *mnemonic, operand_t *operands, uint8_t *operand_count);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
