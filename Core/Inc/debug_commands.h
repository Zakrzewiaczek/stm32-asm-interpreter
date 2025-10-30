#ifndef DEBUG_COMMANDS_H
#define DEBUG_COMMANDS_H

#include "errors.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Check if input is a debug command (starts with '.')
     * @param input Command string
     * @return true if it's a debug command, false otherwise
     */
    bool is_debug_command(const char *input);

    /**
     * @brief Execute debug command
     * @param input Command string (without leading '.')
     * @return Result with error code
     *
     * Supported commands:
     * - .regs          - Display all registers (R0-R7)
     * - .flags         - Display CPU flags (N, Z, C, V)
     * - .cpu           - Display registers + flags
     * - .mem <addr>    - Display 32-bit value at address (hex)
     * - .mem <start> <end> - Display memory range (hex dump)
     * - .clear         - Clear all registers and flags
     * - .reset         - Reset CPU state
     * - .dump          - Full dump: registers, flags, stack info
     */
    result_t execute_debug_command(const char *input);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_COMMANDS_H */
