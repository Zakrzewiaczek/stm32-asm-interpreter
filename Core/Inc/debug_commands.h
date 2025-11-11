/**
 * @file    debug_commands.h
 * @brief   Interactive debug command interface for REPL
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Provides debug commands with '.' prefix for inspecting and controlling
 * the ARM interpreter state. Commands include register display, memory
 * dump, flag inspection, log level control, and system reset.
 */

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
     * @brief Check if input is a debug command
     *
     * Debug commands start with '.' prefix (e.g., ".regs", ".mem").
     *
     * @param[in] input  Input string to check
     *
     * @retval true   Input is a debug command
     * @retval false  Input is not a debug command or is NULL
     */
    bool is_debug_command(const char *input);

    /**
     * @brief Parse and execute debug command
     *
     * Parses the command name and arguments, then dispatches to the
     * appropriate handler function.
     *
     * @param[in] input  Complete debug command string (including '.' prefix)
     *
     * @retval OK                                Command executed successfully
     * @retval ERR_NULL_POINTER                  Input is NULL
     * @retval ERR_INVALID_PARAMETER             Invalid command format
     * @retval ERR_VALIDATE_UNKNOWN_INSTRUCTION  Unknown debug command
     * @retval ERR_*                             Command-specific error
     */
    result_t execute_debug_command(const char *input);

    /**
     * @brief Display CPU register values (.regs)
     *
     * Shows all general-purpose registers (R0-R15) with special notation
     * for SP (R13), LR (R14), and PC (R15). Can optionally show a single
     * register if specified.
     *
     * @param[in] args  Optional register name (e.g., "r0") or NULL for all
     *
     * @retval OK                      Registers displayed successfully
     * @retval ERR_INVALID_PARAMETER   Invalid register name
     */
    result_t cmd_regs(const char *args);

    /**
     * @brief Display APSR condition flags (.flags)
     *
     * Shows current state of N, Z, C, V flags in the APSR register.
     *
     * @retval OK  Flags displayed successfully
     */
    result_t cmd_flags(void);

    /**
     * @brief Display complete CPU state (.cpu)
     *
     * Combines register and flag display for full CPU state overview.
     *
     * @retval OK  CPU state displayed successfully
     */
    result_t cmd_cpu(void);

    /**
     * @brief Get or set log level (.log)
     *
     * Without arguments, displays current log level. With numeric argument
     * (0-4), sets new log level.
     *
     * @param[in] args  New log level (0-4) or NULL to display current
     *
     * @retval OK                      Log level operation successful
     * @retval ERR_INVALID_PARAMETER   Invalid log level value
     */
    result_t cmd_log(const char *args);

    /**
     * @brief Memory inspection and dump (.mem)
     *
     * Display memory contents in multiple formats:
     * - No args: Show memory layout info
     * - Single address: Show value at address
     * - Two addresses: Show memory range dump
     *
     * @param[in] args  Address or address range specification
     *
     * @retval OK                      Memory displayed successfully
     * @retval ERR_INVALID_PARAMETER   Invalid address format
     * @retval ERR_MEM_*               Memory access error
     */
    result_t cmd_mem(const char *args);

    /**
     * @brief Clear all general-purpose registers (.clear)
     *
     * Sets R0-R7 to zero. Does not affect SP, LR, PC, or flags.
     *
     * @retval OK  Registers cleared successfully
     */
    result_t cmd_clear(void);

    /**
     * @brief Reset CPU state (.reset)
     *
     * Clears all registers (R0-R7) and APSR flags. Initializes stack.
     * Does not affect memory contents.
     *
     * @retval OK  CPU reset completed successfully
     */
    result_t cmd_reset(void);

    /**
     * @brief Complete system state dump (.dump)
     *
     * Displays comprehensive system information including registers,
     * flags, memory layout, and stack state.
     *
     * @retval OK  System dump completed successfully
     */
    result_t cmd_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_COMMANDS_H */
