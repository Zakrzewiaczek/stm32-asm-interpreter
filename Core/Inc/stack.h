/**
 * @file    stack.h
 * @brief   ARM Cortex-M4 stack management (Full Descending)
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Implements ARM Full Descending (FD) stack convention:
 * - Stack grows downward (toward lower addresses)
 * - SP points to last occupied location
 * - PUSH: decrement SP, then store value
 * - POP: load value, then increment SP
 *
 * The stack region is reserved at the top of interpreter RAM and
 * managed independently from the memory allocation system.
 */

#ifndef STACK_H
#define STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32l476xx.h"
#include "errors.h"
#include "memory.h"
#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @defgroup Stack_Region Stack Memory Layout
     * @brief Stack region boundaries and configuration
     * @{
     */

#define STACK_TOP (RAM_END)                 ///< Highest stack address (initial SP value)
#define STACK_BOTTOM (RAM_END - STACK_SIZE) ///< Lowest valid stack address

    /** @} */

    /**
     * @brief Initialize stack subsystem
     *
     * Sets the stack pointer (R13) to STACK_TOP and prepares the
     * stack for operation. Must be called before any stack operations.
     */
    void stack_init(void);

    /**
     * @brief Push 32-bit value onto stack (ARM FD convention)
     *
     * Performs: SP = SP - 4; MEM[SP] = value
     *
     * @param[in] value  32-bit value to push onto stack
     *
     * @retval OK                    Push successful
     * @retval ERR_STACK_OVERFLOW    Stack full (SP would go below STACK_BOTTOM)
     * @retval ERR_MEM_*             Memory write error
     */
    result_t stack_push(uint32_t value);

    /**
     * @brief Pop 32-bit value from stack (ARM FD convention)
     *
     * Performs: value = MEM[SP]; SP = SP + 4
     *
     * @param[out] out_value  Pointer to store popped value
     *
     * @retval OK                    Pop successful
     * @retval ERR_NULL_POINTER      out_value is NULL
     * @retval ERR_STACK_UNDERFLOW   Stack empty (SP at or above STACK_TOP)
     * @retval ERR_MEM_*             Memory read error
     */
    result_t stack_pop(uint32_t *out_value);

    /**
     * @brief Get current stack pointer value
     *
     * @return Current SP register value (R13)
     */
    uint32_t stack_get_sp(void);

    /**
     * @brief Get stack depth in bytes
     *
     * Calculates how many bytes are currently used on the stack.
     *
     * @return Number of bytes used (STACK_TOP - current_SP)
     */
    uint32_t stack_get_depth(void);

    /**
     * @brief Check if address is within stack bounds
     *
     * Validates that an address falls within the reserved stack region.
     *
     * @param[in] addr  Address to validate
     *
     * @retval true   Address is within stack bounds
     * @retval false  Address is outside stack region
     */
    bool stack_is_valid_address(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* STACK_H */
