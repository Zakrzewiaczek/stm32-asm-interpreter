/**
 * @file    memory.h
 * @brief   Safe memory access API for ARM interpreter
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Provides safe memory read/write operations with automatic validation
 * and alignment checking. Supports access to interpreter RAM and
 * peripheral regions with bounds checking and error reporting.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "stm32l476xx.h"
#include "log.h"
#include "errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief RAM region start address (from linker script)
     */
    extern uint8_t __interpreter_ram_start__;

    /**
     * @brief RAM region end address (from linker script)
     */
    extern uint8_t __interpreter_ram_end__;

    /** @defgroup Memory_Regions Memory Region Definitions
     * @brief Memory layout constants for validation
     * @{
     */

#define PERIPH_SIZE 0x10000000UL                         ///< Peripheral region size (256 MB)
#define RAM_START ((uint32_t)&__interpreter_ram_start__) ///< Interpreter RAM start address
#define RAM_END ((uint32_t)&__interpreter_ram_end__)     ///< Interpreter RAM end address
#define RAM_SIZE (RAM_END - RAM_START)                   ///< Interpreter RAM size

    /** @} */

    /**
     * @brief Initialize memory subsystem
     *
     * Clears the interpreter RAM region and prepares the memory
     * subsystem for operation.
     */
    void memory_init(void);

    /**
     * @brief Check address alignment for given access size
     *
     * Verifies that the address is properly aligned for the specified
     * access size (1, 2, or 4 bytes).
     *
     * @param[in] addr  Address to check
     * @param[in] size  Access size in bytes (1, 2, or 4)
     *
     * @retval true   Address is properly aligned
     * @retval false  Address is misaligned
     */
    static inline bool mem_is_aligned(uint32_t addr, size_t size)
    {
        return (addr & (size - 1)) == 0;
    }

    /**
     * @brief Validate memory address against valid ranges
     *
     * Checks if the address falls within either the interpreter RAM
     * region or the STM32 peripheral region.
     *
     * @param[in] addr  Address to validate
     *
     * @retval true   Address is within valid memory range
     * @retval false  Address is outside valid ranges
     */
    static inline bool mem_is_address_valid(uint32_t addr)
    {
        // Check interpreter RAM range
        if (addr >= RAM_START && addr < RAM_END)
            return true;

        // Check peripheral range
        if (addr >= PERIPH_BASE && addr < PERIPH_BASE + PERIPH_SIZE)
            return true;

        return false;
    }

    /**
     * @brief Safe 8-bit memory read with validation
     *
     * @param[in]  addr       Memory address to read from
     * @param[out] out_value  Pointer to store the read value
     *
     * @retval OK                        Read successful
     * @retval ERR_NULL_POINTER          out_value is NULL
     * @retval ERR_MEM_INVALID_ADDRESS   Address outside valid ranges
     */
    result_t mem_read8(uint32_t addr, uint8_t *out_value);

    /**
     * @brief Safe 16-bit memory read with validation and alignment check
     *
     * @param[in]  addr       Memory address to read from (must be 2-byte aligned)
     * @param[out] out_value  Pointer to store the read value
     *
     * @retval OK                        Read successful
     * @retval ERR_NULL_POINTER          out_value is NULL
     * @retval ERR_MEM_INVALID_ADDRESS   Address outside valid ranges
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 2-byte aligned
     */
    result_t mem_read16(uint32_t addr, uint16_t *out_value);

    /**
     * @brief Safe 32-bit memory read with validation and alignment check
     *
     * @param[in]  addr       Memory address to read from (must be 4-byte aligned)
     * @param[out] out_value  Pointer to store the read value
     *
     * @retval OK                        Read successful
     * @retval ERR_NULL_POINTER          out_value is NULL
     * @retval ERR_MEM_INVALID_ADDRESS   Address outside valid ranges
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 4-byte aligned
     */
    result_t mem_read32(uint32_t addr, uint32_t *out_value);

    /**
     * @brief Safe 8-bit memory write with validation
     *
     * @param[in] addr   Memory address to write to
     * @param[in] value  Value to write
     *
     * @retval OK                        Write successful
     * @retval ERR_MEM_INVALID_ADDRESS   Address outside valid ranges
     */
    result_t mem_write8(uint32_t addr, uint8_t value);

    /**
     * @brief Safe 16-bit memory write with validation and alignment check
     *
     * @param[in] addr   Memory address to write to (must be 2-byte aligned)
     * @param[in] value  Value to write
     *
     * @retval OK                        Write successful
     * @retval ERR_MEM_INVALID_ADDRESS   Address outside valid ranges
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 2-byte aligned
     */
    result_t mem_write16(uint32_t addr, uint16_t value);

    /**
     * @brief Safe 32-bit memory write with validation and alignment check
     *
     * @param[in] addr   Memory address to write to (must be 4-byte aligned)
     * @param[in] value  Value to write
     *
     * @retval OK                        Write successful
     * @retval ERR_MEM_INVALID_ADDRESS   Address outside valid ranges
     * @retval ERR_MEM_UNALIGNED_ACCESS  Address not 4-byte aligned
     */
    result_t mem_write32(uint32_t addr, uint32_t value);

    /**
     * @brief Specifies whether a warning should be displayed when writing memory to the stack scope.
     */
    extern bool warnOnStackWrite;

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */
