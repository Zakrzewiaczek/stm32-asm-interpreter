

#include "log.h"
#include <stdio.h>
#include <stdarg.h>

static log_level_t log_level = LOG_LEVEL_WARNING;

void set_log_level(log_level_t level)
{
    log_level = level;
}

log_level_t get_log_level(void)
{
    return log_level;
}

bool validate_log_level(int level)
{
    return (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_DEBUG);
}

void log_printf(log_level_t level, const char *fmt, ...)
{
    // Check if the level is sufficient
    if (level > log_level && level != LOG_LEVEL_CRITICAL)
        return;

    // Set the prefix
    const char *prefix = "";
    switch (level)
    {
    case LOG_LEVEL_CRITICAL:
        prefix = "[CRITICAL]";
        break;
    case LOG_LEVEL_ERROR:
        prefix = "[ERROR]";
        break;
    case LOG_LEVEL_WARNING:
        prefix = "[WARN]";
        break;
    case LOG_LEVEL_DEBUG:
        prefix = "[DEBUG]";
        break;
    default:
        break;
    }

    va_list args;
    va_start(args, fmt);

    // Temporary buffer to hold the formatted message
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    printf("%s %s\r\n", prefix, buffer);

    va_end(args);
}
