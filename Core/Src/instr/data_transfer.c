/**
 * @file data_transfer.c
 * @brief Data transfer instruction implementations
 *
 * Implements ARM data transfer instructions (MOV, LDR, STR variants).
 * Supports multiple addressing modes with automatic writeback handling.
 */

#include "instr/data_transfer.h"
#include "cpu.h"
#include "memory.h"
#include "log.h"

/**
 * @brief MOV instruction - Move data to register
 *
 * Moves data from register or immediate value to destination register.
 * Supports:
 * - MOV Rd, Rm     (register to register)
 * - MOV Rd, #imm   (immediate to register)
 *
 * @note Operand validation is performed before execution, so no runtime checks needed
 *
 * @param operands Operand array: [0]=dest_reg, [1]=source (reg or immediate)
 * @param operand_count Number of operands (unused, always 2)
 * @return OK on success, error code otherwise
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
        LOG_DEBUG("MOV: R%d <- R%d (0x%08lX)", dest_reg, src_reg, (unsigned long)value);
        break;
    }

    case OPERAND_IMMEDIATE:
        value = operands[1].value.immediate;
        LOG_DEBUG("MOV: R%d <- #%lu (0x%08lX)", dest_reg, (unsigned long)value, (unsigned long)value);
        break;

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, operands[1].type);
    }

    // Store result in destination register
    cpu.R[dest_reg] = value;
    LOG_DEBUG("MOV: R%d = 0x%08lX", dest_reg, (unsigned long)value);
    return OK;
}

/**
 * @brief MOVS instruction - Move with flags update
 *
 * Same as MOV but updates N (negative) and Z (zero) flags based on result.
 * Negative flag (N) is set if bit 31 of result is 1.
 * Zero flag (Z) is set if result equals 0.
 *
 * @param operands Operand array: [0]=dest_reg, [1]=source
 * @param operand_count Number of operands
 * @return OK on success, error code otherwise
 */
result_t instr_movs(const operand_t *operands, uint8_t operand_count)
{
    uint8_t dest_reg = operands[0].value.reg;

    // Execute MOV first
    result_t res = instr_mov(operands, operand_count);
    if (is_error(res))
        return res; // Propagate error

    // Update N (Negative) and Z (Zero) flags based on result
    uint32_t result = cpu.R[dest_reg];
    cpu.flags &= ~(CPU_FLAG_N_Msk | CPU_FLAG_Z_Msk);

    if (result & 0x80000000)
        cpu.flags |= CPU_FLAG_N_Msk; // Negative flag (bit 31 set)

    if (result == 0)
        cpu.flags |= CPU_FLAG_Z_Msk; // Zero flag

    LOG_DEBUG("MOVS: Updating flags: N->%d, Z->%d", CPU_FLAG_N != 0, CPU_FLAG_Z != 0);
    return OK;
}

/**
 * @brief Internal function to load data from memory
 *
 * Unified implementation for LDR/LDRH/LDRB instructions.
 * Handles all five addressing modes:
 * - [Rn]           Simple
 * - [Rn, #offset]  Offset
 * - [Rn, Rm]       Register offset
 * - [Rn, #offset]! Pre-indexed with writeback
 * - [Rn], #offset  Post-indexed with writeback
 *
 * @param operands Operand array: [0]=dest_reg, [1]=memory_operand
 * @param operand_count Number of operands (unused)
 * @param size Number of bytes to read (1, 2, or 4)
 * @return OK on success, error code otherwise
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

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, operands[1].type);
    }

    // Store result in destination register
    cpu.R[dest_reg] = value;
    LOG_DEBUG("%s: R%d = 0x%08lX", instr_name, dest_reg, (uint32_t)value);
    return OK;
}

result_t instr_ldrb(const operand_t *operands, uint8_t operand_count)
{
    return load_from_memory(operands, operand_count, 1);
}

result_t instr_ldrh(const operand_t *operands, uint8_t operand_count)
{
    return load_from_memory(operands, operand_count, 2);
}

result_t instr_ldr(const operand_t *operands, uint8_t operand_count)
{
    return load_from_memory(operands, operand_count, 4);
}

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
        LOG_DEBUG("%s: mem[0x%08lX] = 0x%08lX", instr_name, (uint32_t)addr, (uint32_t)value);
        break;
    }
        /*
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
        */

    default:
        RAISE_ERR(ERR_EXEC_INVALID_OPERAND_TYPE, operands[1].type);
    }
    return OK;
}

result_t instr_strb(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 1);
}
result_t instr_strh(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 2);
}
result_t instr_str(const operand_t *operands, uint8_t operand_count)
{
    return write_to_memory(operands, operand_count, 4);
}