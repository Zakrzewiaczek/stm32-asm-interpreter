

#include "errors.h"
#include "log.h"
#include <stdio.h>

const char *error_code_to_string(error_code_t code)
{
    switch (code)
    {
    // Success
    case ERR_OK:
        return "OK";

    // General errors
    case ERR_NULL_POINTER:
        return "Null pointer passed to function";
    case ERR_INVALID_PARAMETER:
        return "Invalid parameter value";
    case ERR_NOT_IMPLEMENTED:
        return "Feature not yet implemented";
    case ERR_INTERNAL:
        return "Internal error";

    // Parser errors
    case ERR_PARSE_EMPTY_MNEMONIC:
        return "Empty or missing instruction mnemonic";
    case ERR_PARSE_INVALID_OPERAND:
        return "Invalid operand syntax";
    case ERR_PARSE_MISSING_OPERAND:
        return "Expected operand but found none";
    case ERR_PARSE_TOO_MANY_OPERANDS:
        return "Too many operands";
    case ERR_PARSE_INVALID_REGISTER:
        return "Invalid register name or number";
    case ERR_PARSE_INVALID_IMMEDIATE:
        return "Invalid immediate value format";
    case ERR_PARSE_IMMEDIATE_NO_HASH:
        return "Immediate must start with '#'";
    case ERR_PARSE_INVALID_IMMEDIATE_CHAR:
        return "Invalid character in immediate value";
    case ERR_PARSE_EMPTY_MEMORY:
        return "Empty memory operand []";
    case ERR_PARSE_MISSING_BASE_REG:
        return "Missing base register in memory operand";
    case ERR_PARSE_INVALID_BASE_REG:
        return "Invalid base register in memory operand";
    case ERR_PARSE_UNSUPPORTED_OPERATOR:
        return "Unsupported operator in expression";
    case ERR_PARSE_EMPTY_OFFSET:
        return "Empty offset after comma";
    case ERR_PARSE_EXPECTED_IMMEDIATE:
        return "Expected immediate after comma";
    case ERR_PARSE_UNMATCHED_BRACKET:
        return "Missing closing bracket";
    case ERR_PARSE_DOUBLE_COMMA:
        return "Unexpected double comma";
    case ERR_PARSE_TRAILING_COMMA:
        return "Trailing comma at end";
    case ERR_PARSE_MISSING_COMMA:
        return "Missing comma between operands";
    case ERR_PARSE_IMMEDIATE_OUT_OF_RANGE:
        return "Immediate value exceeds 32-bit range";
    case ERR_PARSE_INVALID_REG_LIST:
        return "Invalid register list syntax";
    case ERR_PARSE_EMPTY_REG_LIST:
        return "Empty register list {}";
    case ERR_PARSE_INVALID_REG_RANGE:
        return "Invalid register range (e.g., R5-R2)";

    // Validation errors
    case ERR_VALIDATE_UNKNOWN_INSTRUCTION:
        return "Unknown instruction mnemonic";
    case ERR_VALIDATE_TOO_FEW_OPERANDS:
        return "Too few operands for instruction";
    case ERR_VALIDATE_TOO_MANY_OPERANDS:
        return "Too many operands for instruction";
    case ERR_VALIDATE_INVALID_OPERAND_TYPE:
        return "Invalid operand type for this instruction";
    case ERR_VALIDATE_CUSTOM_FAILED:
        return "Custom validation failed";

    // Memory errors
    case ERR_MEM_INVALID_ADDRESS:
        return "Address outside valid memory ranges";
    case ERR_MEM_UNALIGNED_ACCESS:
        return "Unaligned memory access";
    case ERR_MEM_READ_FAILED:
        return "Memory read operation failed";
    case ERR_MEM_WRITE_FAILED:
        return "Memory write operation failed";
    case ERR_MEM_ACCESS_VIOLATION:
        return "Access to protected memory";
    case ERR_MEM_OUT_OF_BOUNDS:
        return "Address exceeds memory bounds";

    // Stack errors
    case ERR_STACK_OVERFLOW:
        return "Stack overflow - SP below STACK_BOTTOM";
    case ERR_STACK_UNDERFLOW:
        return "Stack underflow - SP at or above STACK_TOP";
    case ERR_STACK_UNALIGNED:
        return "Stack pointer not 4-byte aligned";
    case ERR_STACK_CORRUPTED:
        return "Stack corruption detected";

    // Execution errors
    case ERR_EXEC_INVALID_REGISTER:
        return "Invalid register number in execution";
    case ERR_EXEC_INVALID_OPERAND_TYPE:
        return "Invalid operand type in execution";
    case ERR_EXEC_DIVISION_BY_ZERO:
        return "Division by zero";
    case ERR_EXEC_OVERFLOW:
        return "Arithmetic overflow";
    case ERR_EXEC_UNDERFLOW:
        return "Arithmetic underflow";
    case ERR_EXEC_LABEL_NOT_FOUND:
        return "Label not found";
    case ERR_EXEC_INVALID_OPERATION:
        return "Invalid operation for current state";

    // CPU/Register errors
    case ERR_CPU_INVALID_REGISTER:
        return "Invalid CPU register access";
    case ERR_CPU_INVALID_FLAG:
        return "Invalid CPU flag access";
    case ERR_CPU_STATE_INVALID:
        return "Invalid CPU state";

    default:
        return "Unknown error code";
    }
}

void log_error_result(result_t res, const char *func_name)
{
    if (res.code == ERR_OK)
    {
        return; // Nothing to log for success
    }

    const char *error_msg = error_code_to_string(res.code);

    // Build context string if context is non-zero
    char context_str[64] = "";
    if (res.context != 0)
    {
        snprintf(context_str, sizeof(context_str), " [context: 0x%08lX]", (uint32_t)res.context);
    }

    // Build additional message if provided
    const char *additional_msg = (res.message != NULL) ? res.message : "";

    // Log with appropriate level based on error severity

    // CRITICAL: System bugs and fatal conditions that should never happen in production
    if (res.code == ERR_NULL_POINTER || // Bug: passing NULL to function
        res.code == ERR_INTERNAL)       // Internal inconsistency/bug
    {
        LOG_CRITICAL("%s [0x%02X]: %s%s %s", func_name, res.code, error_msg, context_str, additional_msg);
    }
    else
    {
        LOG_ERROR("%s [0x%02X]: %s%s %s", func_name, res.code, error_msg, context_str, additional_msg);
    }
}