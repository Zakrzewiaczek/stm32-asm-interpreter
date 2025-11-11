

#include "parser.h"
#include "cpu.h"
#include "log.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

static const char *skip_spaces(const char *p)
{
    while (*p && isspace((uint8_t)*p))
        p++;
    return p;
}

static int parse_register(const char *p, size_t *out_consumed)
{
    const char *start = p;
    if (!p)
        return -1;

    // Accept lowercase/uppercase rN format
    if ((p[0] == 'r' || p[0] == 'R') && isdigit((uint8_t)p[1]))
    {
        char *endptr;
        int32_t rn = strtol(p + 1, &endptr, 10);
        if (rn < 0 || rn > 15)
            return -1;
        if (out_consumed)
            *out_consumed = (size_t)(endptr - start);
        return (int)rn;
    }

    if ((p[0] == 's' || p[0] == 'S') && (p[1] == 'p' || p[1] == 'P'))
    {
        if (out_consumed)
            *out_consumed = 2;
        return 13;
    }

    return -1;
}

// Parse immediate: must start with '#', supports decimal and 0x hex
static result_t parse_immediate(const char *p, uint32_t *out_value, size_t *out_consumed)
{
    if (!p || !out_value)
        RAISE_ERR(ERR_NULL_POINTER, 0);

    if (*p != '#')
    {
        RAISE_ERR(ERR_PARSE_IMMEDIATE_NO_HASH, (uint32_t)(p - p)); // Position 0
    }

    p++; // skip '#'
    const char *num_start = p;

    // Reset errno before calling strtoul
    errno = 0;
    char *endptr;
    uint32_t val = strtoul(p, &endptr, 0);

    if (endptr == p)
    {
        RAISE_ERR(ERR_PARSE_INVALID_IMMEDIATE, 0);
    }

    // Check if there are invalid characters after the number (e.g., 0.01, 0x1g)
    if (*endptr != '\0' && *endptr != ',' && *endptr != ']' && !isspace((uint8_t)*endptr) && *endptr != ';' && *endptr != '@')
    {
        RAISE_ERR(ERR_PARSE_INVALID_IMMEDIATE_CHAR, (uint32_t)*endptr);
    }

    // Check if value exceeds 32-bit range
    if (errno == ERANGE)
    {
        RAISE_ERR(ERR_PARSE_IMMEDIATE_OUT_OF_RANGE, (uint32_t)val);
    }

    *out_value = (uint32_t)val;
    if (out_consumed)
        *out_consumed = (size_t)(endptr - num_start + 1); // include '#'
    return OK;
}

// Parse memory operand: [<reg>] or [<reg>, #<imm>] with optional spaces after commas
static result_t parse_memory_operand(const char *p, operand_t *op, size_t *out_consumed)
{
    const char *start = p;
    if (*p != '[')
        RAISE_ERR(ERR_PARSE_INVALID_OPERAND, 0);

    p++; // skip '['

    p = skip_spaces(p);

    // Check for empty brackets []
    if (*p == ']')
    {
        RAISE_ERR(ERR_PARSE_EMPTY_MEMORY, (uint32_t)(p - start));
    }

    // Check for missing base register (e.g., [, r2])
    if (*p == ',')
    {
        RAISE_ERR(ERR_PARSE_MISSING_BASE_REG, (uint32_t)(p - start));
    }

    size_t consumed = 0;
    int base = parse_register(p, &consumed);

    /*
     * If base is not a register try to parse a symbolic register name
     * (e.g. RCC_MODER). This enables syntax like: [RCC_MODER]
     * The symbol is stored in op->value.label and operand type is
     * OPERAND_MEM_SYMBOL. No offsets or writeback are supported with
     * symbolic bracketed operands (for now).
     */
    if (base < 0)
    {
        const char *id_start = p;
        size_t id_len = 0;
        while (isalnum((uint8_t)*p) || *p == '_')
        {
            p++;
            id_len++;
        }

        if (id_len == 0)
        {
            RAISE_ERR(ERR_PARSE_INVALID_BASE_REG, (uint32_t)(p - start));
        }

        p = skip_spaces(p);

        if (*p != ']')
        {
            // Symbol inside brackets must be alone: [SYMBOL]
            RAISE_ERR(ERR_PARSE_INVALID_OPERAND, (uint32_t)(id_start - start));
        }

        // Advance p past ']'
        p++;

        // Set operand type FIRST before touching union members
        op->type = OPERAND_MEM_SYMBOL;

        // Copy symbol name into label field (truncate if necessary)
        // IMPORTANT: This must be done AFTER setting type and INSTEAD of setting memory fields
        // because operand_t uses a union - label and memory share the same memory space
        size_t copy_len = (id_len < sizeof(op->value.label) - 1) ? id_len : (sizeof(op->value.label) - 1);
        memcpy(op->value.label, id_start, copy_len);
        op->value.label[copy_len] = '\0';

        if (out_consumed)
            *out_consumed = (size_t)(p - start);

        return OK;
    }

    p += consumed;
    p = skip_spaces(p);

    // Check for unsupported operators (+, *, -, (, etc.)
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(')
    {
        RAISE_ERR(ERR_PARSE_UNSUPPORTED_OPERATOR, (uint32_t)*p);
    }

    uint32_t offset = 0;
    uint8_t offset_reg = 0xFF; // 0xFF means no register offset

    if (*p == ',')
    {
        p++; // skip ','
        p = skip_spaces(p);

        // Check for empty offset after comma (e.g., [r1,])
        if (*p == ']')
        {
            RAISE_ERR(ERR_PARSE_EMPTY_OFFSET, (uint32_t)(p - start));
        }

        // Try to parse as register first (e.g., [r0, r1])
        size_t reg_consumed = 0;
        int offset_reg_num = parse_register(p, &reg_consumed);

        if (offset_reg_num >= 0)
        {
            // It's a register offset
            offset_reg = (uint8_t)offset_reg_num;
            p += reg_consumed;
            p = skip_spaces(p);
        }
        else
        {
            // Not a register, must be immediate
            if (*p != '#')
            {
                RAISE_ERR(ERR_PARSE_EXPECTED_IMMEDIATE, (uint32_t)(p - start));
            }

            size_t imm_consumed = 0;
            result_t res = parse_immediate(p, &offset, &imm_consumed);
            if (is_error(res))
                return res;

            p += imm_consumed;
            p = skip_spaces(p);
        }

        // Check again for unsupported operators after offset
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(')
        {
            RAISE_ERR(ERR_PARSE_UNSUPPORTED_OPERATOR, (uint32_t)*p);
        }
    }

    if (*p != ']')
    {
        RAISE_ERR(ERR_PARSE_UNMATCHED_BRACKET, (uint32_t)(p - start));
    }
    p++; // skip ']'

    // Check for writeback modes
    bool writeback = false;
    operand_type_t mem_type = OPERAND_MEM_SIMPLE;

    p = skip_spaces(p);

    // Check for pre-indexed with writeback: [Rn, #imm]!
    if (*p == '!')
    {
        mem_type = OPERAND_MEM_PRE_INC;
        writeback = true;
        p++;
    }
    // Check for post-indexed: [Rn], #imm (but only if there was no offset inside brackets)
    else if (*p == ',' && offset == 0 && offset_reg == 0xFF)
    {
        p++; // skip ','
        p = skip_spaces(p);

        if (*p != '#')
        {
            RAISE_ERR(ERR_PARSE_EXPECTED_IMMEDIATE, (uint32_t)(p - start));
        }

        size_t imm_consumed = 0;
        result_t res = parse_immediate(p, &offset, &imm_consumed);
        if (is_error(res))
            return res;

        p += imm_consumed;
        mem_type = OPERAND_MEM_POST_INC;
        writeback = true;
    }
    // Determine addressing mode based on what was parsed
    else if (offset_reg != 0xFF)
    {
        mem_type = OPERAND_MEM_REG_OFFSET;
    }
    else if (offset != 0)
    {
        mem_type = OPERAND_MEM_OFFSET;
    }
    else
    {
        mem_type = OPERAND_MEM_SIMPLE;
    }

    op->type = mem_type;
    op->value.memory.base_reg = (uint8_t)base;
    op->value.memory.offset = offset;
    op->value.memory.offset_reg = offset_reg;
    op->value.memory.writeback = writeback;
    if (out_consumed)
        *out_consumed = (size_t)(p - start);

    return OK;
}

// Parse a single operand (register, immediate, memory)
static result_t parse_operand(const char *p, operand_t *op, size_t *out_consumed)
{
    p = skip_spaces(p);

    // Check for empty operand (comma or end)
    if (*p == '\0' || *p == ',' || *p == ';' || *p == '@')
    {
        RAISE_ERR(ERR_PARSE_MISSING_OPERAND, 0);
    }

    // memory
    if (*p == '[')
    {
        return parse_memory_operand(p, op, out_consumed);
    }

    // immediate
    if (*p == '#')
    {
        uint32_t val = 0;
        size_t consumed = 0;
        result_t res = parse_immediate(p, &val, &consumed);
        if (is_error(res))
            return res;

        op->type = OPERAND_IMMEDIATE;
        op->value.immediate = val;
        if (out_consumed)
            *out_consumed = consumed;
        return OK;
    }

    // register
    size_t reg_consumed = 0;
    int reg = parse_register(p, &reg_consumed);
    if (reg >= 0)
    {
        op->type = OPERAND_REGISTER;
        op->value.reg = (uint8_t)reg;
        if (out_consumed)
            *out_consumed = reg_consumed;
        return OK;
    }

    RAISE_ERR(ERR_PARSE_INVALID_OPERAND, 0);
}

// Public parse_instruction implementation: tokenizes mnemonic and comma-separated operands
result_t parse_instruction(const char *input, char *mnemonic, operand_t *operands, uint8_t *operand_count)
{
    if (!input || !mnemonic || !operands || !operand_count)
    {
        RAISE_ERR(ERR_NULL_POINTER, 0);
    }

    *operand_count = 0;
    mnemonic[0] = '\0';

    const char *p = input;
    p = skip_spaces(p);
    if (*p == '\0' || *p == ';' || *p == '@')
    {
        RAISE_ERR(ERR_PARSE_EMPTY_MNEMONIC, 0);
    }

    // parse mnemonic (alpha chars only)
    int mi = 0;
    while (*p && !isspace((uint8_t)*p) && *p != ',' && mi < 15)
        mnemonic[mi++] = (char)tolower((uint8_t)*p++);
    mnemonic[mi] = '\0';

    if (mi == 0)
    {
        RAISE_ERR(ERR_PARSE_EMPTY_MNEMONIC, 0);
    }

    p = skip_spaces(p);
    // if end or comment, done
    if (*p == '\0' || *p == ';' || *p == '@')
        return OK;

    // parse operands separated by commas
    while (*p && *p != ';' && *p != '@')
    {
        if (*operand_count >= 3)
        {
            RAISE_ERR(ERR_PARSE_TOO_MANY_OPERANDS, *operand_count);
        }

        size_t consumed = 0;
        result_t res = parse_operand(p, &operands[*operand_count], &consumed);
        if (is_error(res))
            return res;

        (*operand_count)++;
        p += consumed;
        p = skip_spaces(p);

        // Check what comes next
        if (*p == ',')
        {
            p++; // skip comma
            p = skip_spaces(p);

            // Check for double comma or trailing comma
            if (*p == ',')
            {
                RAISE_ERR(ERR_PARSE_DOUBLE_COMMA, (uint32_t)(p - input));
            }
            if (*p == '\0' || *p == ';' || *p == '@')
            {
                RAISE_ERR(ERR_PARSE_TRAILING_COMMA, (uint32_t)(p - input));
            }
            continue; // parse next operand
        }
        else if (*p == '\0' || *p == ';' || *p == '@')
        {
            break; // end of operands
        }
        else
        {
            // There's something after operand but it's not a comma
            RAISE_ERR(ERR_PARSE_MISSING_COMMA, 0);
        }
    }

    return OK;
}
