/**
 * @file log.c
 * @brief Logging system implementation
 *
 * Provides runtime-configurable logging with severity levels.
 * Log output is sent to stdout (redirected to UART in main.c).
 */

#include "log.h"
#include <stdio.h>
#include <stdarg.h>

/** Current global log level */
static log_level_t log_level = LOG_LEVEL_WARNING;

/**
 * @brief Set global log level
 * @param level New log level
 */
void set_log_level(log_level_t level)
{
    log_level = level;
}

/**
 * @brief Get current log level
 * @return Current log level
 */
log_level_t get_log_level(void)
{
    return log_level;
}

/**
 * @brief Validate if integer is valid log level
 * @param level Integer to validate
 * @return true if valid, false otherwise
 */
bool validate_log_level(int level)
{
    return (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_DEBUG);
}

/**
 * @brief Print formatted log message
 *
 * Outputs message only if its level is <= current log level.
 * Adds severity prefix to message.
 *
 * @param level Message severity level
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
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
