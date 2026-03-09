

#include "commands.h"
#include "log.h"
#include "instr/commands_includes.h"
#include <string.h>

const instruction_descriptor_t instructions[] = {
    // Data transfer instructions
    {"mov", 2, 2, {OP_REG, OP_REG | OP_IMM}, NULL, instr_mov},
    {"movs", 2, 2, {OP_REG, OP_REG | OP_IMM}, NULL, instr_movs},

    {"ldr", 2, 2, {OP_REG, OP_MEM_ALL}, NULL, instr_ldr},
    {"ldrb", 2, 2, {OP_REG, OP_MEM_ALL}, NULL, instr_ldrb},
    {"ldrh", 2, 2, {OP_REG, OP_MEM_ALL}, NULL, instr_ldrh},

    {"str", 2, 2, {OP_REG, OP_MEM_ALL}, NULL, instr_str},
    {"strb", 2, 2, {OP_REG, OP_MEM_ALL}, NULL, instr_strb},
    {"strh", 2, 2, {OP_REG, OP_MEM_ALL}, NULL, instr_strh},

    {"push", 1, 1, {OP_REG | OP_REG_LIST}, NULL, instr_push},
    {"pop", 1, 1, {OP_REG | OP_REG_LIST}, NULL, instr_pop},

    // Sentinel (marks end of table)
    {NULL, 0, 0, {0}, NULL, NULL}};

/**
 * @brief Find an instruction descriptor by mnemonic.
 */
static const instruction_descriptor_t *find_instruction(const char *mnemonic)
{
    for (int i = 0; instructions[i].mnemonic != NULL; i++)
    {
        if (strcmp(instructions[i].mnemonic, mnemonic) == 0)
        {
            return &instructions[i];
        }
    }
    return NULL;
}

result_t validate_instruction(const char *mnemonic, const operand_t *operands, uint8_t operand_count)
{
    if (!mnemonic || !operands)
    {
        RAISE_ERR(ERR_NULL_POINTER, 0);
    }

    const instruction_descriptor_t *desc = find_instruction(mnemonic);
    if (!desc)
    {
        RAISE_ERR(ERR_VALIDATE_UNKNOWN_INSTRUCTION, 0);
    }

    // Check operand count
    if (operand_count < desc->min_operands)
    {
        RAISE_ERR(ERR_VALIDATE_TOO_FEW_OPERANDS, operand_count);
    }
    if (operand_count > desc->max_operands)
    {
        RAISE_ERR(ERR_VALIDATE_TOO_MANY_OPERANDS, operand_count);
    }

    // Check operand types
    for (uint8_t i = 0; i < operand_count; i++)
    {
        uint32_t allowed = desc->operand_patterns[i];
        operand_type_t actual = operands[i].type;

        if (!(allowed & actual))
        {
            RAISE_ERR(ERR_VALIDATE_INVALID_OPERAND_TYPE, (i << 8) | actual);
        }
    }

    // Execute custom validation (if provided)
    if (desc->validate)
    {
        result_t res = desc->validate(operands, operand_count);
        if (is_error(res))
        {
            return res;
        }
    }

    return OK;
}

result_t execute_instruction(const char *mnemonic, const operand_t *operands, uint8_t operand_count)
{
    if (!mnemonic || !operands)
    {
        RAISE_ERR(ERR_NULL_POINTER, 0);
    }

    const instruction_descriptor_t *desc = find_instruction(mnemonic);
    if (!desc)
    {
        RAISE_ERR(ERR_VALIDATE_UNKNOWN_INSTRUCTION, 0);
    }

    if (!desc->execute)
    {
        RAISE_ERR(ERR_NOT_IMPLEMENTED, 0);
    }

    return desc->execute(operands, operand_count);
}