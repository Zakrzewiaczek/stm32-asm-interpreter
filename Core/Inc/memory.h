/**
 * @file memory.h
 * @brief Safe memory access API with validation
 *
 * Provides safe read/write operations for 8/16/32-bit memory access with
 * automatic address validation and alignment checking.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32l476xx.h"
#include "errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup Memory_Constants Memory Range Constants
 * @{
 */

/** Size of peripheral memory region (256 MB) */
#define PERIPH_SIZE 0x10000000

    /** @} */

    /** @defgroup Memory_Symbols Linker Script Symbols
     * @brief Memory region boundaries defined in linker script
     * @{
     */

    /** RAM starting address (linker script symbol) */
    extern uint8_t __interpreter_ram_start__;
    /** RAM ending address (linker script symbol) */
    extern uint8_t __interpreter_ram_end__;

/** @} */

/** @defgroup Memory_Macros RAM Region Macros
 * @{
 */

/** Get RAM start address */
#define RAM_START ((uint32_t)&__interpreter_ram_start__)
/** Get RAM end address */
#define RAM_END ((uint32_t)&__interpreter_ram_end__)
/** Calculate total RAM size */
#define RAM_SIZE (RAM_END - RAM_START)

    /** @} */

    /**
     * @brief Check if address is properly aligned for given size
     * @param addr Address to check
     * @param size Size in bytes (1, 2, or 4)
     * @return true if aligned, false otherwise
     */
    static inline bool mem_is_aligned(uint32_t addr, size_t size)
    {
        return (addr & (size - 1)) == 0;
    }

    /**
     * @brief Check if address is within valid memory ranges
     *
     * Validates that the address falls within either RAM or peripheral memory regions.
     *
     * @param addr Address to validate
     * @return true if address is in valid range, false otherwise
     */
    static inline bool mem_is_address_valid(uint32_t addr)
    {
        // Check if address is in RAM range
        if (addr >= RAM_START && addr < RAM_END)
            return true;

        // Check if address is in peripheral range
        if (addr >= PERIPH_BASE && addr < PERIPH_BASE + PERIPH_SIZE)
            return true;

        return false;
    }

    /**
     * @brief Safe 8-bit memory read with validation
     * @param addr Memory address to read from
     * @param out_value Pointer to store the read value
     * @return Result with error code (ERR_OK on success, error otherwise)
     */
    result_t mem_read8(uint32_t addr, uint8_t *out_value);

    /**
     * @brief Safe 16-bit memory read with validation and alignment check
     *
     * Address must be 2-byte aligned (addr % 2 == 0).
     *
     * @param addr Memory address to read from (must be 2-byte aligned)
     * @param out_value Pointer to store the read value
     * @return Result with error code (ERR_OK on success, error otherwise)
     */
    result_t mem_read16(uint32_t addr, uint16_t *out_value);

    /**
     * @brief Safe 32-bit memory read with validation and alignment check
     *
     * Address must be 4-byte aligned (addr % 4 == 0).
     *
     * @param addr Memory address to read from (must be 4-byte aligned)
     * @param out_value Pointer to store the read value
     * @return Result with error code (ERR_OK on success, error otherwise)
     */
    result_t mem_read32(uint32_t addr, uint32_t *out_value);

    /**
     * @brief Safe 8-bit memory write with validation
     * @param addr Memory address to write to
     * @param value Value to write
     * @return Result with error code (ERR_OK on success, error otherwise)
     */
    result_t mem_write8(uint32_t addr, uint8_t value);

    /**
     * @brief Safe 16-bit memory write with validation and alignment check
     *
     * Address must be 2-byte aligned (addr % 2 == 0).
     *
     * @param addr Memory address to write to (must be 2-byte aligned)
     * @param value Value to write
     * @return Result with error code (ERR_OK on success, error otherwise)
     */
    result_t mem_write16(uint32_t addr, uint16_t value);

    /**
     * @brief Safe 32-bit memory write with validation and alignment check
     *
     * Address must be 4-byte aligned (addr % 4 == 0).
     *
     * @param addr Memory address to write to (must be 4-byte aligned)
     * @param value Value to write
     * @return Result with error code (ERR_OK on success, error otherwise)
     */
    result_t mem_write32(uint32_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */
