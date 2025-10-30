#ifndef ERRORS_H
#define ERRORS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Error codes for the ASM interpreter
     *
     * These codes cover all possible error conditions in the interpreter.
     * They are organized by subsystem: general, parser, validation, memory, execution.
     */
    typedef enum
    {
        // === SUCCESS ===
        ERR_OK = 0x00, // No error

        // === GENERAL ERRORS (0x01-0x0F) ===
        ERR_NULL_POINTER = 0x01,      // Null pointer passed to function
        ERR_INVALID_PARAMETER = 0x02, // Invalid parameter value
        ERR_NOT_IMPLEMENTED = 0x03,   // Feature not yet implemented
        ERR_INTERNAL = 0x04,          // Internal error (should not happen)

        // === PARSER ERRORS (0x10-0x1F) ===
        ERR_PARSE_EMPTY_MNEMONIC = 0x10,         // Empty or missing instruction mnemonic
        ERR_PARSE_INVALID_OPERAND = 0x11,        // Invalid operand syntax
        ERR_PARSE_MISSING_OPERAND = 0x12,        // Expected operand but found none
        ERR_PARSE_TOO_MANY_OPERANDS = 0x13,      // More operands than allowed
        ERR_PARSE_INVALID_REGISTER = 0x14,       // Invalid register name or number
        ERR_PARSE_INVALID_IMMEDIATE = 0x15,      // Invalid immediate value format
        ERR_PARSE_IMMEDIATE_NO_HASH = 0x16,      // Immediate must start with '#'
        ERR_PARSE_INVALID_IMMEDIATE_CHAR = 0x17, // Invalid character in immediate
        ERR_PARSE_EMPTY_MEMORY = 0x18,           // Empty memory operand []
        ERR_PARSE_MISSING_BASE_REG = 0x19,       // Missing base register in memory operand
        ERR_PARSE_INVALID_BASE_REG = 0x1A,       // Invalid base register in memory operand
        ERR_PARSE_UNSUPPORTED_OPERATOR = 0x1B,   // Unsupported operator in expression
        ERR_PARSE_EMPTY_OFFSET = 0x1C,           // Empty offset after comma
        ERR_PARSE_EXPECTED_IMMEDIATE = 0x1D,     // Expected immediate after comma
        ERR_PARSE_UNMATCHED_BRACKET = 0x1E,      // Missing closing bracket
        ERR_PARSE_DOUBLE_COMMA = 0x1F,           // Unexpected double comma
        ERR_PARSE_TRAILING_COMMA = 0x20,         // Trailing comma at end
        ERR_PARSE_MISSING_COMMA = 0x21,          // Missing comma between operands
        ERR_PARSE_IMMEDIATE_OUT_OF_RANGE = 0x22, // Immediate value exceeds 32-bit range

        // === VALIDATION ERRORS (0x30-0x3F) ===
        ERR_VALIDATE_UNKNOWN_INSTRUCTION = 0x30,  // Unknown instruction mnemonic
        ERR_VALIDATE_TOO_FEW_OPERANDS = 0x31,     // Too few operands for instruction
        ERR_VALIDATE_TOO_MANY_OPERANDS = 0x32,    // Too many operands for instruction
        ERR_VALIDATE_INVALID_OPERAND_TYPE = 0x33, // Operand type not allowed for this instruction
        ERR_VALIDATE_CUSTOM_FAILED = 0x34,        // Custom validation function failed

        // === MEMORY ERRORS (0x50-0x5F) ===
        ERR_MEM_INVALID_ADDRESS = 0x50,  // Address outside valid memory ranges
        ERR_MEM_UNALIGNED_ACCESS = 0x51, // Unaligned memory access (16/32-bit)
        ERR_MEM_READ_FAILED = 0x52,      // Memory read operation failed
        ERR_MEM_WRITE_FAILED = 0x53,     // Memory write operation failed
        ERR_MEM_ACCESS_VIOLATION = 0x54, // Access to protected/forbidden memory
        ERR_MEM_OUT_OF_BOUNDS = 0x55,    // Address exceeds memory bounds

        // === EXECUTION ERRORS (0x70-0x7F) ===
        ERR_EXEC_INVALID_REGISTER = 0x70,     // Invalid register number in execution
        ERR_EXEC_INVALID_OPERAND_TYPE = 0x71, // Invalid operand type in execution
        ERR_EXEC_DIVISION_BY_ZERO = 0x72,     // Division or modulo by zero
        ERR_EXEC_OVERFLOW = 0x73,             // Arithmetic overflow
        ERR_EXEC_UNDERFLOW = 0x74,            // Arithmetic underflow
        ERR_EXEC_LABEL_NOT_FOUND = 0x75,      // Label not found
        ERR_EXEC_STACK_OVERFLOW = 0x76,       // Stack overflow
        ERR_EXEC_STACK_UNDERFLOW = 0x77,      // Stack underflow
        ERR_EXEC_INVALID_OPERATION = 0x78,    // Invalid operation for current state

        // === CPU/REGISTER ERRORS (0x90-0x9F) ===
        ERR_CPU_INVALID_REGISTER = 0x90, // Invalid CPU register access
        ERR_CPU_INVALID_FLAG = 0x91,     // Invalid CPU flag access
        ERR_CPU_STATE_INVALID = 0x92,    // Invalid CPU state

        // === SENTINEL (for future expansion) ===
        ERR_MAX = 0xFF // Maximum error code value
    } error_code_t;

    /**
     * @brief Result type for functions that can fail
     *
     * Contains error code and optional context information.
     * Use the OK and ERR() macros for convenient creation.
     */
    typedef struct
    {
        error_code_t code;   // Error code (ERR_OK if successful)
        uint32_t context;    // Context-dependent value (e.g., address, register number, line position)
        const char *message; // Optional additional message (can be NULL)
    } result_t;

    /**
     * @brief Create a successful result
     */
#define OK ((result_t){.code = ERR_OK, .context = 0, .message = NULL})

    /**
     * @brief Create an error result with context
     * @param err_code Error code
     * @param ctx Context value
     */
#define ERR(err_code, ctx) ((result_t){.code = (err_code), .context = (ctx), .message = NULL})

    /**
     * @brief Create an error result with context and message
     * @param err_code Error code
     * @param ctx Context value
     * @param msg Error message
     */
#define ERR_MSG(err_code, ctx, msg) ((result_t){.code = (err_code), .context = (ctx), .message = (msg)})

    /**
     * @brief Check if result is successful
     * @param res Result to check
     * @return true if OK, false if error
     */
#define is_ok(res) ((res).code == ERR_OK)

    /**
     * @brief Check if result is error
     * @param res Result to check
     * @return true if error, false if OK
     */
#define is_error(res) ((res).code != ERR_OK)

    /**
     * @brief Get human-readable error message for error code
     * @param code Error code
     * @return Constant string describing the error
     */
    const char *error_code_to_string(error_code_t code);

    /**
     * @brief Log error result with full context
     * @param res Result to log
     * @param func_name Name of function where error occurred
     */
    void log_error_result(result_t res, const char *func_name);

    /**
     * @brief Log and return an error in one step.
     *
     * Usage: RAISE_ERR(ERR_PARSE_INVALID_OPERAND, pos);
     * This creates a result_t, logs it (with the current function name) and returns it.
     */
#define RAISE_ERR(err_code, ctx)                \
    do                                          \
    {                                           \
        result_t _res = ERR((err_code), (ctx)); \
        log_error_result(_res, __func__);       \
        return _res;                            \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* ERRORS_H */
