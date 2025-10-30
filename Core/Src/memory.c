/**
 * @file memory.c
 * @brief Safe memory access implementation
 *
 * Implements validated memory read/write operations for 8/16/32-bit access.
 */

#include <stddef.h>
#include "memory.h"
#include "log.h"

// TODO: Implement memory sectors with read/write permissions
// When attempting write to read-only or read from write-only, return ERR_MEM_ACCESS_VIOLATION

/**
 * @brief Safe 8-bit memory read with validation
 */
result_t mem_read8(uint32_t addr, uint8_t *out_value)
{
    if (out_value == NULL)
    {
        RAISE_ERR(ERR_NULL_POINTER, addr);
    }

    if (!mem_is_address_valid(addr))
    {
        RAISE_ERR(ERR_MEM_INVALID_ADDRESS, addr);
    }

    // Perform read
    *out_value = *(volatile uint8_t *)addr;
    LOG_DEBUG("mem_read8: [0x%08lX] = 0x%02X", (uint32_t)addr, *out_value);
    return OK;
}

/**
 * @brief Safe 16-bit memory read with validation and alignment check
 */
result_t mem_read16(uint32_t addr, uint16_t *out_value)
{
    if (out_value == NULL)
    {
        RAISE_ERR(ERR_NULL_POINTER, addr);
    }

    if (!mem_is_address_valid(addr))
    {
        RAISE_ERR(ERR_MEM_INVALID_ADDRESS, addr);
    }

    if (!mem_is_aligned(addr, 2))
    {
        RAISE_ERR(ERR_MEM_UNALIGNED_ACCESS, addr);
    }

    // Perform read
    *out_value = *(volatile uint16_t *)addr;
    LOG_DEBUG("mem_read16: [0x%08lX] = 0x%04X", (uint32_t)addr, *out_value);
    return OK;
}

/**
 * @brief Safe 32-bit memory read with validation and alignment check
 */
result_t mem_read32(uint32_t addr, uint32_t *out_value)
{
    if (out_value == NULL)
    {
        RAISE_ERR(ERR_NULL_POINTER, addr);
    }

    if (!mem_is_address_valid(addr))
    {
        RAISE_ERR(ERR_MEM_INVALID_ADDRESS, addr);
    }

    if (!mem_is_aligned(addr, 4))
    {
        RAISE_ERR(ERR_MEM_UNALIGNED_ACCESS, addr);
    }

    // Perform read
    *out_value = *(volatile uint32_t *)addr;
    LOG_DEBUG("mem_read32: [0x%08lX] = 0x%08lX", (uint32_t)addr, (uint32_t)*out_value);
    return OK;
}

/**
 * @brief Safe 8-bit memory write with validation
 */
result_t mem_write8(uint32_t addr, uint8_t value)
{
    if (!mem_is_address_valid(addr))
    {
        RAISE_ERR(ERR_MEM_INVALID_ADDRESS, addr);
    }

    // Perform write
    *(volatile uint8_t *)addr = value;
    LOG_DEBUG("mem_write8: [0x%08lX] <- 0x%02X", (uint32_t)addr, value);
    return OK;
}

/**
 * @brief Safe 16-bit memory write with validation and alignment check
 */
result_t mem_write16(uint32_t addr, uint16_t value)
{
    if (!mem_is_address_valid(addr))
    {
        RAISE_ERR(ERR_MEM_INVALID_ADDRESS, addr);
    }

    if (!mem_is_aligned(addr, 2))
    {
        RAISE_ERR(ERR_MEM_UNALIGNED_ACCESS, addr);
    }

    // Perform write
    *(volatile uint16_t *)addr = value;
    LOG_DEBUG("mem_write16: [0x%08lX] <- 0x%04X", (uint32_t)addr, value);
    return OK;
}

/**
 * @brief Safe 32-bit memory write with validation and alignment check
 */
result_t mem_write32(uint32_t addr, uint32_t value)
{
    if (!mem_is_address_valid(addr))
    {
        RAISE_ERR(ERR_MEM_INVALID_ADDRESS, addr);
    }

    if (!mem_is_aligned(addr, 4))
    {
        RAISE_ERR(ERR_MEM_UNALIGNED_ACCESS, addr);
    }

    // Perform write
    *(volatile uint32_t *)addr = value;
    LOG_DEBUG("mem_write32: [0x%08lX] <- 0x%08lX", (uint32_t)addr, (uint32_t)value);
    return OK;
}
