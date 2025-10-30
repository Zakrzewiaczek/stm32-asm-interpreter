/**
 * @file debug_commands.c
 * @brief Debug command handlers for REPL interface
 *
 * Implements interactive debug commands starting with '.' prefix:
 * - Register inspection (.regs)
 * - Memory dump (.mem)
 * - Flag display (.flags)
 * - Log level control (.log)
 * - CPU reset (.reset)
 * - Screen clear (.clear)
 * - Help display (.help)
 */

#include "debug_commands.h"
#include "cpu.h"
#include "memory.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @brief Skip leading whitespace characters
 * @param p Input string pointer
 * @return Pointer to first non-whitespace character
 */
static const char *skip_spaces(const char *p)
{
    while (*p && isspace((uint8_t)*p))
        p++;
    return p;
}

/**
 * @brief Parse hexadecimal address from string
 *
 * Accepts addresses in format: 0x1234ABCD, 1234ABCD, etc.
 *
 * @param str Input string
 * @param out_addr Output parameter for parsed address
 * @return true on success, false on parse error
 */
static bool parse_hex_address(const char *str, uint32_t *out_addr)
{
    if (!str || !out_addr)
        return false;

    // Skip leading whitespace
    while (*str && isspace((uint8_t)*str))
        str++;

    char *endptr;
    errno = 0;                                // Reset errno before strtoul
    uint32_t addr = strtoul(str, &endptr, 0); // Supports 0x prefix

    // Check if parsing succeeded (endptr moved forward)
    if (endptr == str)
        return false;

    // Check for overflow (strtoul sets errno to ERANGE)
    if (errno == ERANGE || addr > 0xFFFFFFFFUL)
        return false;

    // Check if there are trailing non-whitespace characters
    while (*endptr && isspace((uint8_t)*endptr))
        endptr++;
    if (*endptr != '\0')
        return false; // Trailing garbage detected

    *out_addr = addr;
    return true;
}

bool is_debug_command(const char *input)
{
    if (!input)
        return false;

    const char *p = skip_spaces(input);
    return (*p == '.');
}

// Display registers (all or specific)
// Usage: .regs      - display all registers
//        .regs <reg>   - display only <reg>
static result_t cmd_regs(const char *args)
{
    // Check if specific register requested
    if (args && *args != '\0')
    {
        const char *p = skip_spaces(args);

        // Parse register name (r0, r1, ..., r7 or R0, R1, ..., R7)
        if ((p[0] == 'r' || p[0] == 'R') && p[1] >= '0' && p[1] <= '7' && (p[2] == '\0' || isspace((uint8_t)p[2])))
        {
            int reg_num = p[1] - '0';
            printf("R%d -> 0x%08lX (%lu)\r\n", reg_num, (uint32_t)cpu.R[reg_num], (uint32_t)cpu.R[reg_num]);
            return OK;
        }
        else
        {
            printf("Error: Invalid register name. Use r0-r7 or R0-R7\r\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }
    }

    // Display all registers
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int index = j + i * 4;
            if (index >= sizeof(cpu.R) / sizeof(cpu.R[0]))
                break;
            printf("R%d%s -> 0x%08lX   ", index, (index < 10) ? " " : "", (uint32_t)cpu.R[index]);
        }
        printf("\r\n");
    }
    return OK;
}

// Display flags
static result_t cmd_flags(void)
{
    printf("APSR = 0x%08lX\r\n", (uint32_t)cpu.flags);
    printf("  N -> %ld\r\n", (int32_t)CPU_FLAG_N);
    printf("  Z -> %ld\r\n", (int32_t)CPU_FLAG_Z);
    printf("  C -> %ld\r\n", (int32_t)CPU_FLAG_C);
    printf("  V -> %ld\r\n", (int32_t)CPU_FLAG_V);
    return OK;
}

// Display CPU state (registers + flags)
static result_t cmd_cpu(void)
{
    printf("[Registers]\r\n");
    cmd_regs(NULL);
    printf("\r\n[Flags]\r\n");
    cmd_flags();
    return OK;
}

// Set log level: .log <level>
static result_t cmd_log(const char *args)
{
    if (!args || *args == '\0')
    {
        // Display current log level
        log_level_t current = get_log_level();
        log_printf(current, "Current log level: %d\r\n", current);
        return OK;
    }

    // Parse level
    const char *p = skip_spaces(args);
    log_level_t new_level = LOG_LEVEL_NONE;

    if (isdigit((uint8_t)*p))
    {
        int level_num = atoi(p);
        if (!validate_log_level(level_num))
        {
            printf("Error: Incorrect log level\r\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, level_num);
            return ERR(ERR_INVALID_PARAMETER, level_num);
        }
        new_level = (log_level_t)level_num;
    }
    else
    {
        printf("Error: Unknown log level '%s'\r\n", p);
        RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        return ERR(ERR_INVALID_PARAMETER, 0);
    }

    set_log_level(new_level);
    log_printf(new_level, "Setting log level to: %d\r\n", new_level);
    return OK;
}

// Display memory at address or range
static result_t cmd_mem(const char *args)
{
    args = skip_spaces(args);

    if (*args == '\0')
    {
        printf("RAM start addr:    %p\r\n", (void *)RAM_START);
        printf("RAM end addr:      %p\r\n", (void *)(RAM_END - 1));
        printf("RAM size:          %lu bytes (0x%lX)\r\n", (uint32_t)RAM_SIZE, (uint32_t)RAM_SIZE);
        printf("PERIPH start addr: %p\r\n", (void *)PERIPH_BASE);
        printf("PERIPH end addr:   %p\r\n", (void *)(PERIPH_BASE + PERIPH_SIZE - 1));
        printf("PERIPH size:       %lu bytes (0x%lX)\r\n", (uint32_t)PERIPH_SIZE, (uint32_t)PERIPH_SIZE);
        return OK;
    }

    // Parse first address
    char *space_pos = strchr(args, ' ');
    char addr_str[32];
    uint32_t start_addr = 0;

    if (space_pos)
    {
        // Two arguments: start and end
        size_t len = (size_t)(space_pos - args);
        if (len >= sizeof(addr_str))
            len = sizeof(addr_str) - 1;
        strncpy(addr_str, args, len);
        addr_str[len] = '\0';

        if (!parse_hex_address(addr_str, &start_addr))
        {
            printf("Invalid start address\r\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        // Parse end address
        const char *end_str = skip_spaces(space_pos + 1);
        uint32_t end_addr = 0;
        if (!parse_hex_address(end_str, &end_addr))
        {
            printf("Invalid end address\r\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        if (end_addr < start_addr)
        {
            printf("End address must be >= start address\r\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        // Display memory range
        printf("\r\nmem [0x%08lX - 0x%08lX] \r\n", (uint32_t)start_addr, (uint32_t)end_addr);

        log_level_t previous_level = get_log_level();
        set_log_level(LOG_LEVEL_WARNING); // Suppress mem_read32 debug logs

        uint32_t addr = start_addr & ~0x3; // Align to 4 bytes
        if (addr != start_addr)
            LOG_WARN("Start address 0x%08lX is not 4-byte aligned, aligned down to 0x%08lX", (uint32_t)start_addr, (uint32_t)addr);
        if (end_addr & 0x3)
        {
            LOG_WARN("End address 0x%08lX is not 4-byte aligned, aligned down to 0x%08lX", (uint32_t)end_addr, (uint32_t)(end_addr & ~0x3));
            end_addr &= ~0x3;
        }
        printf("\r\n");
        while (addr <= end_addr)
        {
            uint32_t value = 0;
            result_t res = mem_read32(addr, &value);

            if (is_ok(res))
            {
                // Extract individual bytes (little-endian)
                uint8_t b0 = (value >> 0) & 0xFF;
                uint8_t b1 = (value >> 8) & 0xFF;
                uint8_t b2 = (value >> 16) & 0xFF;
                uint8_t b3 = (value >> 24) & 0xFF;

                // Display: address, 32-bit value, individual bytes, ASCII
                printf("0x%08lX: 0x%08lX    %02X %02X %02X %02X    ",
                       (uint32_t)addr, (uint32_t)value,
                       b0, b1, b2, b3);

                // ASCII representation (printable chars only)
                for (int i = 0; i < 4; i++)
                {
                    uint8_t byte = (value >> (i * 8)) & 0xFF;
                    if (byte >= 32 && byte <= 126)
                        printf("%c", byte);
                    else
                        printf(".");
                }
                printf("\r\n");
            }
            else
            {
                printf("0x%08lX: <read error>\r\n", (uint32_t)addr);
            }

            addr += 4;

            // Safety limit: max 64 words
            if ((addr - start_addr) > 256)
            {
                printf("... (truncated, max 64 words)\r\n");
                break;
            }
        }
        set_log_level(previous_level);
    }
    else
    {
        // Single address
        if (!parse_hex_address(args, &start_addr))
        {
            printf("Invalid address\r\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        uint32_t value = 0;

        log_level_t previous_level = get_log_level();
        set_log_level(LOG_LEVEL_WARNING); // Suppress mem_read32 debug logs
        result_t res = mem_read32(start_addr, &value);
        set_log_level(previous_level);

        if (is_error(res))
        {
            return res; // Propagate error (already logged)
        }

        printf("mem [0x%08lX] -> 0x%08lX (%lu)\r\n\r\n",
               (uint32_t)start_addr,
               (uint32_t)value,
               (uint32_t)value);
    }

    return OK;
}

// Clear all registers
static result_t cmd_clear(void)
{
    for (int i = 0; i < 8; i++)
    {
        cpu.R[i] = 0;
    }
    printf("Registers cleared\r\n");
    return OK;
}

// Reset CPU state
static result_t cmd_reset(void)
{
    for (int i = 0; i < 8; i++)
    {
        cpu.R[i] = 0;
    }
    cpu.flags = 0;
    printf("CPU reset done\r\n");
    return OK;
}

// Full system dump
static result_t cmd_dump(void)
{
    printf("[Registers]\r\n");
    cmd_regs(NULL);
    printf("\r\n[Flags]\r\n");
    cmd_flags();
    printf("\r\n[Memory]\r\n");
    cmd_mem(NULL);

    return OK;
}

result_t execute_debug_command(const char *input)
{
    if (!input)
        RAISE_ERR(ERR_NULL_POINTER, 0);

    const char *p = skip_spaces(input);

    // Skip '.' prefix
    if (*p != '.')
        RAISE_ERR(ERR_INVALID_PARAMETER, 0);
    p++;

    // Parse command name
    const char *cmd_start = p;
    while (*p && !isspace((uint8_t)*p))
        p++;

    size_t cmd_len = (size_t)(p - cmd_start);
    char cmd[32];
    if (cmd_len >= sizeof(cmd))
        cmd_len = sizeof(cmd) - 1;
    strncpy(cmd, cmd_start, cmd_len);
    cmd[cmd_len] = '\0';

    // Convert to lowercase
    for (size_t i = 0; i < cmd_len; i++)
    {
        cmd[i] = (char)tolower((uint8_t)cmd[i]);
    }

    // Skip to arguments
    p = skip_spaces(p);

    // Dispatch command
    if (strcmp(cmd, "regs") == 0)
        return cmd_regs(p); // Pass arguments (may be NULL or specific register)
    else if (strcmp(cmd, "flags") == 0)
        return cmd_flags();
    else if (strcmp(cmd, "cpu") == 0)
        return cmd_cpu();
    else if (strcmp(cmd, "mem") == 0)
        return cmd_mem(p);
    else if (strcmp(cmd, "log") == 0)
        return cmd_log(p);
    else if (strcmp(cmd, "clear") == 0)
        return cmd_clear();
    else if (strcmp(cmd, "reset") == 0)
        return cmd_reset();
    else if (strcmp(cmd, "dump") == 0)
        return cmd_dump();
    else
    {
        printf("Unknown debug command: .%s\r\n", cmd);
        printf("Available: .regs .flags .cpu .mem .log .clear .reset .dump\r\n");
        RAISE_ERR(ERR_NOT_IMPLEMENTED, 0);
    }
}
