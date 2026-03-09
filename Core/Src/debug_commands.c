#include "debug_commands.h"
#include "cpu.h"
#include "memory.h"
#include "stack.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

static const char *skip_spaces(const char *p)
{
    while (*p && isspace((uint8_t)*p))
        p++;
    return p;
}

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

static const char *reg_alias(int index)
{
    switch (index)
    {
    case 13:
        return "SP";
    case 14:
        return "LR";
    case 15:
        return "PC";
    default:
        return NULL;
    }
}

static int parse_debug_register(const char *p)
{
    if (!p)
        return -1;
    if ((p[0] == 's' || p[0] == 'S') && (p[1] == 'p' || p[1] == 'P') && (p[2] == '\0' || isspace((uint8_t)p[2])))
        return 13;
    if ((p[0] == 'l' || p[0] == 'L') && (p[1] == 'r' || p[1] == 'R') && (p[2] == '\0' || isspace((uint8_t)p[2])))
        return 14;
    if ((p[0] == 'p' || p[0] == 'P') && (p[1] == 'c' || p[1] == 'C') && (p[2] == '\0' || isspace((uint8_t)p[2])))
        return 15;
    if ((p[0] == 'r' || p[0] == 'R') && isdigit((uint8_t)p[1]))
    {
        int reg_num = p[1] - '0';
        if (isdigit((uint8_t)p[2]))
        {
            reg_num = reg_num * 10 + (p[2] - '0');
            if (p[3] != '\0' && !isspace((uint8_t)p[3]))
                return -1;
        }
        else if (p[2] != '\0' && !isspace((uint8_t)p[2]))
        {
            return -1;
        }
        if (reg_num < 0 || reg_num > 15)
            return -1;
        return reg_num;
    }
    return -1;
}

result_t cmd_regs(const char *args)
{
    // Check if specific register requested
    if (args && *args != '\0')
    {
        const char *p = skip_spaces(args);
        int reg_num = parse_debug_register(p);

        if (reg_num >= 0)
        {
            const char *alias = reg_alias(reg_num);
            if (alias)
                printf("%s (R%d) -> 0x%08lX (%lu)\n", alias, reg_num, (uint32_t)cpu.R[reg_num],
                       (uint32_t)cpu.R[reg_num]);
            else
                printf("R%d -> 0x%08lX (%lu)\n", reg_num, (uint32_t)cpu.R[reg_num], (uint32_t)cpu.R[reg_num]);
            return OK;
        }
        else
        {
            printf("Error: Invalid register name. Use r0-r15, sp, lr, pc\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }
    }

    // Display all registers
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int index = j + i * 4;
            if (index >= (int)(sizeof(cpu.R) / sizeof(cpu.R[0])))
                break;

            const char *alias = reg_alias(index);
            if (alias)
                printf("%s  -> 0x%08lX   ", alias, (uint32_t)cpu.R[index]);
            else
                printf("R%d%s -> 0x%08lX   ", index, (index < 10) ? " " : "", (uint32_t)cpu.R[index]);
        }
        printf("\n");
    }
    return OK;
}

result_t cmd_flags(void)
{
    printf("APSR = 0x%08lX\n", (uint32_t)cpu.flags);
    printf("  N -> %ld\n", (int32_t)CPU_FLAG_N);
    printf("  Z -> %ld\n", (int32_t)CPU_FLAG_Z);
    printf("  C -> %ld\n", (int32_t)CPU_FLAG_C);
    printf("  V -> %ld\n", (int32_t)CPU_FLAG_V);
    return OK;
}

result_t cmd_cpu(void)
{
    printf("[Registers]\n");
    cmd_regs(NULL);
    printf("\n[Flags]\n");
    cmd_flags();
    return OK;
}

result_t cmd_log(const char *args)
{
    if (!args || *args == '\0')
    {
        // Display current log level
        log_level_t current = get_log_level();
        log_printf(current, "Current log level: %d\n", current);
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
            printf("Error: Incorrect log level\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, level_num);
        }
        new_level = (log_level_t)level_num;
    }
    else
    {
        printf("Error: Unknown log level '%s'\n", p);
        RAISE_ERR(ERR_INVALID_PARAMETER, 0);
    }

    set_log_level(new_level);
    log_printf(new_level, "Setting log level to: %d\n", new_level);
    return OK;
}

result_t cmd_mem(const char *args)
{
    args = skip_spaces(args);

    if (*args == '\0')
    {
        printf("RAM start addr:    %p\n", (void *)RAM_START);
        printf("RAM end addr:      %p\n", (void *)(RAM_END - 1));
        printf("RAM size:          %lu bytes (0x%lX)\n", (uint32_t)RAM_SIZE, (uint32_t)RAM_SIZE);
        printf("STACK top addr:    %p\n", (void *)(STACK_TOP - 1));
        printf("STACK bottom addr: %p\n", (void *)STACK_BOTTOM);
        printf("STACK size:        %lu bytes (0x%lX)\n", (uint32_t)STACK_SIZE, (uint32_t)STACK_SIZE);
        printf("PERIPH start addr: %p\n", (void *)PERIPH_BASE);
        printf("PERIPH end addr:   %p\n", (void *)(PERIPH_BASE + PERIPH_SIZE - 1));
        printf("PERIPH size:       %lu bytes (0x%lX)\n", (uint32_t)PERIPH_SIZE, (uint32_t)PERIPH_SIZE);
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
            printf("Invalid start address\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        // Parse end address
        const char *end_str = skip_spaces(space_pos + 1);
        uint32_t end_addr = 0;
        if (!parse_hex_address(end_str, &end_addr))
        {
            printf("Invalid end address\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        if (end_addr < start_addr)
        {
            printf("End address must be >= start address\n");
            RAISE_ERR(ERR_INVALID_PARAMETER, 0);
        }

        // Display memory range
        printf("\nmem [0x%08lX - 0x%08lX] \n", (uint32_t)start_addr, (uint32_t)end_addr);

        log_level_t previous_level = get_log_level();
        set_log_level(LOG_LEVEL_WARNING); // Suppress mem_read32 debug logs

        uint32_t addr = start_addr & ~0x3; // Align to 4 bytes
        if (addr != start_addr)
            LOG_WARN("Start address 0x%08lX is not 4-byte aligned, aligned down to 0x%08lX", (uint32_t)start_addr,
                     (uint32_t)addr);
        if (end_addr & 0x3)
        {
            LOG_WARN("End address 0x%08lX is not 4-byte aligned, aligned down to 0x%08lX", (uint32_t)end_addr,
                     (uint32_t)(end_addr & ~0x3));
            end_addr &= ~0x3;
        }
        printf("\n");
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
                printf("0x%08lX: 0x%08lX    %02X %02X %02X %02X    ", (uint32_t)addr, (uint32_t)value, b0, b1, b2, b3);

                // ASCII representation (printable chars only)
                for (int i = 0; i < 4; i++)
                {
                    uint8_t byte = (value >> (i * 8)) & 0xFF;
                    if (byte >= 32 && byte <= 126)
                        printf("%c", byte);
                    else
                        printf(".");
                }
                printf("\n");
            }
            else
            {
                printf("0x%08lX: <read error>\n", (uint32_t)addr);
            }

            addr += 4;

            // Safety limit: max 64 words
            if ((addr - start_addr) > 256)
            {
                printf("... (truncated, max 64 words)\n");
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
            printf("Invalid address\n");
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

        printf("mem [0x%08lX] -> 0x%08lX (%lu)\n\n", (uint32_t)start_addr, (uint32_t)value, (uint32_t)value);
    }

    return OK;
}

result_t cmd_clear(void)
{
    for (int i = 0; i < 16; i++)
    {
        cpu.R[i] = 0;
    }
    cpu.R[CPU_SP] = STACK_TOP; // Reset SP to top of stack
    printf("Registers cleared\n");
    return OK;
}

result_t cmd_reset(void)
{
    for (int i = 0; i < 16; i++)
    {
        cpu.R[i] = 0;
    }
    cpu.R[CPU_SP] = STACK_TOP; // Reset SP to top of stack
    cpu.flags = 0;
    printf("CPU reset done\n");
    return OK;
}

// Full system dump
result_t cmd_dump(void)
{
    printf("[Registers]\n");
    cmd_regs(NULL);
    printf("\n[Flags]\n");
    cmd_flags();
    printf("\n[Memory]\n");
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
        printf("Unknown debug command: .%s\n", cmd);
        printf("Available: .regs .flags .cpu .mem .log .clear .reset .dump\n");
        RAISE_ERR(ERR_NOT_IMPLEMENTED, 0);
    }
}
