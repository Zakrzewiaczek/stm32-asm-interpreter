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
 * @brief Result of memory address resolution
 */
typedef struct
{
    uint32_t addr;     ///< Resolved effective address
    bool writeback;    ///< Whether base register should be updated
    uint32_t new_base; ///< New base register value (if writeback)
    uint8_t base_reg;  ///< Base register number (for writeback)
} resolved_addr_t;

/**
 * @brief Resolve memory operand to effective address
 *
 * Computes the effective address for all ARM addressing modes.
 * Handles pre/post indexing and writeback. The caller is responsible
 * for actually performing the writeback after the memory access.
 *
 * @param[in]  mem_op    Memory operand to resolve
 * @param[out] resolved  Resolved address information
 * @param[in]  instr_name  Instruction name for debug logging
 * @return Operation result
 */
static result_t resolve_memory_address(const operand_t *mem_op, resolved_addr_t *resolved, const char *instr_name)
{
    resolved->writeback = false;
    resolved->new_base = 0;
    resolved->base_reg = 0;

    switch (mem_op->type)
    {
    case OPERAND_MEM_SIMPLE: // [Rn]
    {
        uint8_t base_reg = mem_op->value.memory.base_reg;
        resolved->addr = cpu.R[base_reg];
        LOG_DEBUG("%s: addr [R%d] = 0x%08lX", instr_name, base_reg, (uint32_t)resolved->addr);
        break;
    }

    case OPERAND_MEM_OFFSET: // [Rn, #offset]
    {
        uint8_t base_reg = mem_op->value.memory.base_reg;
        int32_t offset = mem_op->value.memory.offset;
        resolved->addr = (uint32_t)((int32_t)cpu.R[base_reg] + offset);
        LOG_DEBUG("%s: addr [R%d, #%ld] = 0x%08lX", instr_name, base_reg, (int32_t)offset, (uint32_t)resolved->addr);
        break;
    }

    case OPERAND_MEM_REG_OFFSET: // [Rn, Rm]
    {
        uint8_t base_reg = mem_op->value.memory.base_reg;
        uint8_t offset_reg = mem_op->value.memory.offset_reg;
        resolved->addr = cpu.R[base_reg] + cpu.R[offset_reg];
        LOG_DEBUG("%s: addr [R%d, R%d] = 0x%08lX", instr_name, base_reg, offset_reg, (uint32_t)resolved->addr);
        break;
    }

    case OPERAND_MEM_PRE_INC: // [Rn, #offset]!
    {
        uint8_t base_reg = mem_op->value.memory.base_reg;
        int32_t offset = mem_op->value.memory.offset;
        resolved->addr = (uint32_t)((int32_t)cpu.R[base_reg] + offset);
        resolved->writeback = true;
        resolved->new_base = resolved->addr;
        resolved->base_reg = base_reg;
        LOG_DEBUG("%s: addr [R%d, #%ld]! = 0x%08lX", instr_name, base_reg, (int32_t)offset, (uint32_t)resolved->addr);
        break;
    }

    case OPERAND_MEM_POST_INC: // [Rn], #offset
    {
        uint8_t base_reg = mem_op->value.memory.base_reg;
        int32_t offset = mem_op->value.memory.offset;
        resolved->addr = cpu.R[base_reg];
        resolved->writeback = true;
        resolved->new_base = (uint32_t)((int32_t)cpu.R[base_reg] + offset);
        resolved->base_reg = base_reg;
        LOG_DEBUG("%s: addr [R%d], #%ld = 0x%08lX", instr_name, base_reg, (int32_t)offset, (uint32_t)resolved->addr);
        break;
    }

    case OPERAND_MEM_SYMBOL: // [SYMBOL]
    {
        const char *symbol = mem_op->value.label;
        register_t *reg = get_register(symbol);
        if (!reg)
        {
            LOG_ERROR("%s: Unknown register symbol '%s'", instr_name, symbol);
            RAISE_ERR(ERR_PARSE_INVALID_OPERAND, 0);
        }
        resolved->addr = (uint32_t)reg->address;
        LOG_DEBUG("%s: addr [%s] = 0x%08lX", instr_name, symbol, (uint32_t)resolved->addr);
        break;
    }

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, mem_op->type);
    }

    return OK;
}

/**
 * @brief Apply writeback to base register after memory access
 */
static void apply_writeback(const resolved_addr_t *resolved, const char *instr_name)
{
    if (resolved->writeback)
    {
        uint32_t old = cpu.R[resolved->base_reg];
        cpu.R[resolved->base_reg] = resolved->new_base;
        LOG_DEBUG("%s: R%d writeback: 0x%08lX -> 0x%08lX", instr_name, resolved->base_reg, (uint32_t)old,
                  (uint32_t)resolved->new_base);
    }
}

/**
 * @brief Perform sized memory read
 */
static result_t mem_read_sized(uint32_t addr, uint32_t *out_value, uint8_t size)
{
    *out_value = 0;
    if (size == 1)
        return mem_read8(addr, (uint8_t *)out_value);
    else if (size == 2)
        return mem_read16(addr, (uint16_t *)out_value);
    else
        return mem_read32(addr, out_value);
}

/**
 * @brief Perform sized memory write
 */
static result_t mem_write_sized(uint32_t addr, uint32_t value, uint8_t size)
{
    if (size == 1)
        return mem_write8(addr, (uint8_t)value);
    else if (size == 2)
        return mem_write16(addr, (uint16_t)value);
    else
        return mem_write32(addr, value);
}

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

    LOG_DEBUG("MOVS: Flags updated: N=%d, Z=%d", (cpu.flags & CPU_FLAG_N_Msk) != 0, (cpu.flags & CPU_FLAG_Z_Msk) != 0);
    return OK;
}

/**
 * @brief Generic memory load operation
 *
 * Uses resolve_memory_address() to compute effective address,
 * then performs sized read and optional writeback.
 */
static result_t load_from_memory(const operand_t *operands, uint8_t operand_count, uint8_t size)
{
    (void)operand_count;

    uint8_t dest_reg = operands[0].value.reg;
    const char *instr_name = (size == 1) ? "LDRB" : (size == 2) ? "LDRH" : "LDR";

    resolved_addr_t resolved;
    result_t res = resolve_memory_address(&operands[1], &resolved, instr_name);
    if (is_error(res))
        return res;

    uint32_t value = 0;
    res = mem_read_sized(resolved.addr, &value, size);
    if (is_error(res))
        return res;

    LOG_DEBUG("%s: R%d <- mem[0x%08lX] = 0x%08lX", instr_name, dest_reg, (uint32_t)resolved.addr, (uint32_t)value);

    apply_writeback(&resolved, instr_name);

    cpu.R[dest_reg] = value;
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
 * Uses resolve_memory_address() to compute effective address,
 * then performs sized write and optional writeback.
 */
static result_t write_to_memory(const operand_t *operands, uint8_t operand_count, uint8_t size)
{
    (void)operand_count;

    uint8_t source_reg = operands[0].value.reg;
    uint32_t value = cpu.R[source_reg];
    const char *instr_name = (size == 1) ? "STRB" : (size == 2) ? "STRH" : "STR";

    resolved_addr_t resolved;
    result_t res = resolve_memory_address(&operands[1], &resolved, instr_name);
    if (is_error(res))
        return res;

    res = mem_write_sized(resolved.addr, value, size);
    if (is_error(res))
        return res;

    LOG_DEBUG("%s: mem[0x%08lX] <- R%d = 0x%08lX", instr_name, (uint32_t)resolved.addr, source_reg, (uint32_t)value);

    apply_writeback(&resolved, instr_name);

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
result_t instr_str(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 4);
}

/**
 * @brief PUSH instruction implementation
 *
 * Pushes a single register or a register list onto the stack
 * using ARM Full Descending convention.
 * The stack pointer is decremented before storing each value.
 * Registers are pushed from highest to lowest number.
 */
result_t instr_push(const operand_t *operands, uint8_t operand_count)
{
    (void)operand_count;
    result_t res;

    if (operands[0].type == OPERAND_REG_LIST)
    {
        uint16_t reg_list = operands[0].value.reg_list;
        // Push from highest register to lowest (ARM convention)
        for (int i = 15; i >= 0; i--)
        {
            if (reg_list & (1 << i))
            {
                res = stack_push(cpu.R[i]);
                if (is_error(res))
                    return res;
                LOG_DEBUG("PUSH: R%d = 0x%08lX", i, (uint32_t)cpu.R[i]);
            }
        }
    }
    else
    {
        uint8_t source_reg = operands[0].value.reg;
        res = stack_push(cpu.R[source_reg]);
        if (is_error(res))
            return res;
        LOG_DEBUG("PUSH: R%d = 0x%08lX", source_reg, (uint32_t)cpu.R[source_reg]);
    }

    return OK;
}

/**
 * @brief POP instruction implementation
 *
 * Pops a value or a register list from the stack into registers
 * using ARM Full Descending convention.
 * Values are loaded first, then the stack pointer is incremented.
 * Registers are popped from lowest to highest number.
 */
result_t instr_pop(const operand_t *operands, uint8_t operand_count)
{
    (void)operand_count;
    result_t res;

    if (operands[0].type == OPERAND_REG_LIST)
    {
        uint16_t reg_list = operands[0].value.reg_list;
        // Pop from lowest register to highest (ARM convention)
        for (int i = 0; i <= 15; i++)
        {
            if (reg_list & (1 << i))
            {
                uint32_t value;
                res = stack_pop(&value);
                if (is_error(res))
                    return res;
                cpu.R[i] = value;
                LOG_DEBUG("POP: R%d = 0x%08lX", i, (uint32_t)value);
            }
        }
    }
    else
    {
        uint8_t dest_reg = operands[0].value.reg;
        uint32_t value;
        res = stack_pop(&value);
        if (is_error(res))
            return res;
        cpu.R[dest_reg] = value;
        LOG_DEBUG("POP: R%d = 0x%08lX", dest_reg, (uint32_t)value);
    }

    return OK;
}
