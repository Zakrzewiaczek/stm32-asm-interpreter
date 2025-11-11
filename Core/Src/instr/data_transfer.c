/**
 * @file    data_transfer.c
 * @brief   ARM data transfer instruction implementations
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Implementation of ARM data transfer instructions. Supports all ARM addressing modes and data sizes
 * (8-bit, 16-bit, 32-bit) with comprehensive memory access validation.
 */

#include "instr/data_transfer.h"
#include "cpu.h"
#include "memory.h"
#include "stack.h"
#include "log.h"
#include "stm32l476xx_registers.h"

/**
 * @brief MOV instruction implementation
 *
 * Moves data from source operand to destination register without
 * updating condition flags. Supports register-to-register and
 * immediate-to-register transfers.
 */
result_t instr_mov(const operand_t *operands, uint8_t operand_count)
{
    (void)operand_count; // Unused

    uint8_t dest_reg = operands[0].value.reg;
    uint32_t value = 0;

    switch (operands[1].type)
    {
    case OPERAND_REGISTER:
    {
        uint8_t src_reg = operands[1].value.reg;
        value = cpu.R[src_reg];
        LOG_DEBUG("MOV: R%d <- R%d (0x%08lX)", dest_reg, src_reg, (uint32_t)value);
        break;
    }

    case OPERAND_IMMEDIATE:
        value = operands[1].value.immediate;
        LOG_DEBUG("MOV: R%d <- #%lu (0x%08lX)", dest_reg, (uint32_t)value, (uint32_t)value);
        break;

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, operands[1].type);
    }

    // Store result in destination register
    cpu.R[dest_reg] = value;
    LOG_DEBUG("MOV: R%d = 0x%08lX", dest_reg, (uint32_t)value);
    return OK;
}

/**
 * @brief MOVS instruction implementation
 *
 * Same as MOV but updates condition flags (N and Z) based on the
 * result value. This is useful for conditional execution.
 */
result_t instr_movs(const operand_t *operands, uint8_t operand_count)
{
    uint8_t dest_reg = operands[0].value.reg;

    // Execute MOV operation first
    result_t res = instr_mov(operands, operand_count);
    if (is_error(res))
        return res; // Propagate error

    // Update condition flags based on result
    uint32_t result = cpu.R[dest_reg];
    cpu.flags &= ~(CPU_FLAG_N_Msk | CPU_FLAG_Z_Msk);

    if (result & 0x80000000)
        cpu.flags |= CPU_FLAG_N_Msk; // Negative flag (MSB set)

    if (result == 0)
        cpu.flags |= CPU_FLAG_Z_Msk; // Zero flag

    LOG_DEBUG("MOVS: Flags updated: N=%d, Z=%d",
              (cpu.flags & CPU_FLAG_N_Msk) != 0,
              (cpu.flags & CPU_FLAG_Z_Msk) != 0);
    return OK;
}

/**
 * @brief Generic memory load operation
 *
 * Handles all ARM addressing modes for load operations with different
 * data sizes (8-bit, 16-bit, 32-bit). Supports pre-indexed, post-indexed,
 * and offset addressing modes.
 *
 * @param[in] operands       Array of operands [dest_reg, memory_operand]
 * @param[in] operand_count  Number of operands
 * @param[in] size          Data size in bytes (1, 2, or 4)
 * @return                  Operation result
 */
static result_t load_from_memory(const operand_t *operands, uint8_t operand_count, uint8_t size)
{
    (void)operand_count; // Unused

    uint8_t dest_reg = operands[0].value.reg;
    uint32_t value = 0;
    result_t res;

    const char *instr_name = (size == 1) ? "LDRB" : (size == 2) ? "LDRH"
                                                                : "LDR";

    switch (operands[1].type)
    {
    case OPERAND_MEM_SIMPLE: // [Rn]
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg];

        LOG_DEBUG("%s: Reading from [R%d] = 0x%08lX", instr_name, base_reg, (uint32_t)addr);

        if (size == 1)
            res = mem_read8(addr, (uint8_t *)&value);
        else if (size == 2)
            res = mem_read16(addr, (uint16_t *)&value);
        else
            res = mem_read32(addr, &value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: R%d <- mem[0x%08lX] = 0x%08lX", instr_name, dest_reg, (uint32_t)addr, (uint32_t)value);
        break;
    }

    case OPERAND_MEM_OFFSET: // [Rn, #imm]
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg] + operands[1].value.memory.offset;

        LOG_DEBUG("%s: Reading from [R%d, #%ld] = 0x%08lX",
                  instr_name, base_reg, (int32_t)operands[1].value.memory.offset, (uint32_t)addr);

        if (size == 1)
            res = mem_read8(addr, (uint8_t *)&value);
        else if (size == 2)
            res = mem_read16(addr, (uint16_t *)&value);
        else
            res = mem_read32(addr, &value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: R%d <- mem[0x%08lX] = 0x%08lX", instr_name, dest_reg, (uint32_t)addr, (uint32_t)value);
        break;
    }

    case OPERAND_MEM_REG_OFFSET: // [Rn, Rm]
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint8_t offset_reg = operands[1].value.memory.offset_reg;
        uint32_t addr = cpu.R[base_reg] + cpu.R[offset_reg];

        LOG_DEBUG("%s: Reading from [R%d, R%d] = 0x%08lX",
                  instr_name, base_reg, offset_reg, (uint32_t)addr);

        if (size == 1)
            res = mem_read8(addr, (uint8_t *)&value);
        else if (size == 2)
            res = mem_read16(addr, (uint16_t *)&value);
        else
            res = mem_read32(addr, &value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: R%d <- mem[0x%08lX] = 0x%08lX", instr_name, dest_reg, (uint32_t)addr, (uint32_t)value);
        break;
    }

    case OPERAND_MEM_PRE_INC: // [Rn, #imm]!
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg] + operands[1].value.memory.offset;

        LOG_DEBUG("%s: Reading from [R%d, #%ld]! = 0x%08lX",
                  instr_name, base_reg, (int32_t)operands[1].value.memory.offset, (uint32_t)addr);

        if (size == 1)
            res = mem_read8(addr, (uint8_t *)&value);
        else if (size == 2)
            res = mem_read16(addr, (uint16_t *)&value);
        else
            res = mem_read32(addr, &value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: R%d <- mem[0x%08lX] = 0x%08lX", instr_name, dest_reg, (uint32_t)addr, (uint32_t)value);

        uint32_t old_addr = cpu.R[base_reg];
        cpu.R[base_reg] = addr;
        LOG_DEBUG("%s: R%d writeback: 0x%08lX -> 0x%08lX", instr_name, base_reg, (uint32_t)old_addr, (uint32_t)cpu.R[base_reg]);

        break;
    }

    case OPERAND_MEM_POST_INC: // [Rn], #imm
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg];

        LOG_DEBUG("%s: Reading from [R%d], #%ld = 0x%08lX",
                  instr_name, base_reg, (int32_t)operands[1].value.memory.offset, (uint32_t)addr);

        if (size == 1)
            res = mem_read8(addr, (uint8_t *)&value);
        else if (size == 2)
            res = mem_read16(addr, (uint16_t *)&value);
        else
            res = mem_read32(addr, &value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: R%d <- mem[0x%08lX] = 0x%08lX", instr_name, dest_reg, (uint32_t)addr, (uint32_t)value);

        uint32_t old_addr = cpu.R[base_reg];
        cpu.R[base_reg] += operands[1].value.memory.offset;
        LOG_DEBUG("%s: R%d post-increment: 0x%08lX -> 0x%08lX", instr_name, base_reg, (uint32_t)old_addr, (uint32_t)cpu.R[base_reg]);

        break;
    }

    case OPERAND_MEM_SYMBOL: // [SYMBOL]
    {
        const char *symbol = operands[1].value.label;
        register_t *reg = get_register(symbol);

        if (!reg)
        {
            LOG_ERROR("%s: Unknown register symbol '%s'", instr_name, symbol);
            RAISE_ERR(ERR_PARSE_INVALID_OPERAND, 0);
        }

        uint32_t addr = (uint32_t)reg->address;
        LOG_DEBUG("%s: Reading from [%s] = 0x%08lX", instr_name, symbol, addr);

        if (size == 1)
            res = mem_read8(addr, (uint8_t *)&value);
        else if (size == 2)
            res = mem_read16(addr, (uint16_t *)&value);
        else
            res = mem_read32(addr, &value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: R%d <- mem[%s @ 0x%08lX] = 0x%08lX",
                  instr_name, dest_reg, symbol, addr, (uint32_t)value);
        break;
    }

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, operands[1].type);
    }

    // Store result in destination register
    cpu.R[dest_reg] = value;
    LOG_DEBUG("%s: R%d = 0x%08lX", instr_name, dest_reg, (uint32_t)value);
    return OK;
}

/**
 * @brief LDRB instruction implementation - Load byte
 */
result_t instr_ldrb(const operand_t *operands, uint8_t operand_count)
{
    return load_from_memory(operands, operand_count, 1);
}

/**
 * @brief LDRH instruction implementation - Load halfword
 */
result_t instr_ldrh(const operand_t *operands, uint8_t operand_count)
{
    return load_from_memory(operands, operand_count, 2);
}

/**
 * @brief LDR instruction implementation - Load word
 */
result_t instr_ldr(const operand_t *operands, uint8_t operand_count)
{
    return load_from_memory(operands, operand_count, 4);
}

/**
 * @brief Generic memory store operation
 *
 * Handles all ARM addressing modes for store operations with different
 * data sizes (8-bit, 16-bit, 32-bit). Supports pre-indexed, post-indexed,
 * and offset addressing modes.
 *
 * @param[in] operands       Array of operands [source_reg, memory_operand]
 * @param[in] operand_count  Number of operands
 * @param[in] size          Data size in bytes (1, 2, or 4)
 * @return                  Operation result
 */
static result_t write_to_memory(const operand_t *operands, uint8_t operand_count, uint8_t size)
{
    (void)operand_count; // Unused

    uint8_t source_reg = operands[0].value.reg;
    uint32_t value = cpu.R[source_reg];
    result_t res;

    const char *instr_name = (size == 1) ? "STRB" : (size == 2) ? "STRH"
                                                                : "STR";

    switch (operands[1].type)
    {
    case OPERAND_MEM_SIMPLE: // [Rn]
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg];

        LOG_DEBUG("%s: Reading from [R%d] = 0x%08lX", instr_name, base_reg, (uint32_t)addr);

        if (size == 1)
            res = mem_write8(addr, (uint8_t)value);
        else if (size == 2)
            res = mem_write16(addr, (uint16_t)value);
        else
            res = mem_write32(addr, value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: mem[0x%08lX] <- R%d = 0x%08lX", instr_name, (uint32_t)addr, source_reg, (uint32_t)value);
        break;
    }

    case OPERAND_MEM_OFFSET: // [Rn, #imm]
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg] + operands[1].value.memory.offset;

        LOG_DEBUG("%s: Writing to [R%d, #%ld] = 0x%08lX",
                  instr_name, base_reg, (int32_t)operands[1].value.memory.offset, (uint32_t)addr);

        if (size == 1)
            res = mem_write8(addr, (uint8_t)value);
        else if (size == 2)
            res = mem_write16(addr, (uint16_t)value);
        else
            res = mem_write32(addr, value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: mem[0x%08lX] <- R%d = 0x%08lX", instr_name, (uint32_t)addr, source_reg, (uint32_t)value);
        break;
    }

    case OPERAND_MEM_REG_OFFSET: // [Rn, Rm]
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint8_t offset_reg = operands[1].value.memory.offset_reg;
        uint32_t addr = cpu.R[base_reg] + cpu.R[offset_reg];

        LOG_DEBUG("%s: Writing to [R%d, R%d] = 0x%08lX",
                  instr_name, base_reg, offset_reg, (uint32_t)addr);

        if (size == 1)
            res = mem_write8(addr, (uint8_t)value);
        else if (size == 2)
            res = mem_write16(addr, (uint16_t)value);
        else
            res = mem_write32(addr, value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: mem[0x%08lX] <- R%d = 0x%08lX", instr_name, (uint32_t)addr, source_reg, (uint32_t)value);
        break;
    }

    case OPERAND_MEM_PRE_INC: // [Rn, #imm]!
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg] + operands[1].value.memory.offset;

        LOG_DEBUG("%s: Writing to [R%d, #%ld]! = 0x%08lX",
                  instr_name, base_reg, (int32_t)operands[1].value.memory.offset, (uint32_t)addr);

        if (size == 1)
            res = mem_write8(addr, (uint8_t)value);
        else if (size == 2)
            res = mem_write16(addr, (uint16_t)value);
        else
            res = mem_write32(addr, value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: mem[0x%08lX] <- R%d = 0x%08lX", instr_name, (uint32_t)addr, source_reg, (uint32_t)value);

        uint32_t old_addr = cpu.R[base_reg];
        cpu.R[base_reg] = addr;
        LOG_DEBUG("%s: R%d writeback: 0x%08lX -> 0x%08lX", instr_name, base_reg, (uint32_t)old_addr, (uint32_t)cpu.R[base_reg]);

        break;
    }

    case OPERAND_MEM_POST_INC: // [Rn], #imm
    {
        uint8_t base_reg = operands[1].value.memory.base_reg;
        uint32_t addr = cpu.R[base_reg];

        LOG_DEBUG("%s: Writing to [R%d], #%ld = 0x%08lX",
                  instr_name, base_reg, (int32_t)operands[1].value.memory.offset, (uint32_t)addr);

        if (size == 1)
            res = mem_write8(addr, (uint8_t)value);
        else if (size == 2)
            res = mem_write16(addr, (uint16_t)value);
        else
            res = mem_write32(addr, value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: mem[0x%08lX] <- R%d = 0x%08lX", instr_name, (uint32_t)addr, source_reg, (uint32_t)value);

        uint32_t old_addr = cpu.R[base_reg];
        cpu.R[base_reg] += operands[1].value.memory.offset;
        LOG_DEBUG("%s: R%d post-increment: 0x%08lX -> 0x%08lX", instr_name, base_reg, (uint32_t)old_addr, (uint32_t)cpu.R[base_reg]);

        break;
    }

    case OPERAND_MEM_SYMBOL: // [SYMBOL] - symbolic register name
    {
        const char *symbol = operands[1].value.label;
        register_t *reg = get_register(symbol);

        if (!reg)
        {
            LOG_ERROR("%s: Unknown register symbol '%s'", instr_name, symbol);
            RAISE_ERR(ERR_PARSE_INVALID_OPERAND, 0);
        }

        uint32_t addr = (uint32_t)reg->address;
        LOG_DEBUG("%s: Writing to [%s] = 0x%08lX", instr_name, symbol, addr);

        if (size == 1)
            res = mem_write8(addr, (uint8_t)value);
        else if (size == 2)
            res = mem_write16(addr, (uint16_t)value);
        else
            res = mem_write32(addr, value);

        if (is_error(res))
        {
            return res;
        }

        LOG_DEBUG("%s: mem[%s @ 0x%08lX] <- R%d = 0x%08lX",
                  instr_name, symbol, addr, source_reg, (uint32_t)value);
        break;
    }

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, operands[1].type);
    }
    return OK;
}

/**
 * @brief STRB instruction implementation - Store byte
 */
result_t instr_strb(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 1);
}

/**
 * @brief STRH instruction implementation - Store halfword
 */
result_t instr_strh(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 2);
}

/**
 * @brief STR instruction implementation - Store word
 */
/**
 * @brief STR instruction implementation - Store word
 */
result_t instr_str(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 4);
}

/**
 * @brief PUSH instruction implementation
 *
 * Pushes a register value onto the stack using ARM Full Descending convention.
 * The stack pointer is decremented before storing the value.
 */
result_t instr_push(const operand_t *operands, uint8_t operand_count)
{
    (void)operand_count; // Unused

    uint8_t source_reg = operands[0].value.reg;
    uint32_t value = cpu.R[source_reg];
    result_t res;

    res = stack_push(value);

    if (is_error(res))
    {
        return res;
    }

    return OK;
}

/**
 * @brief POP instruction implementation
 *
 * Pops a value from the stack into a register using ARM Full Descending convention.
 * The value is loaded first, then the stack pointer is incremented.
 */
result_t instr_pop(const operand_t *operands, uint8_t operand_count)
{
    (void)operand_count; // Unused

    uint8_t dest_reg = operands[0].value.reg;
    uint32_t value;
    result_t res;

    res = stack_pop(&value);

    if (is_error(res))
    {
        return res;
    }

    cpu.R[dest_reg] = value;
    return OK;
}
