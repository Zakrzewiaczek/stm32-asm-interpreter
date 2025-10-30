/**
 * @file log.h
 * @brief Logging system with configurable verbosity levels
 *
 * Provides leveled logging output (CRITICAL, ERROR, WARNING, DEBUG).
 * Log level can be changed at runtime to control verbosity.
 */

#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Log level enumeration
     *
     * Defines severity levels for logging. Messages are only printed
     * if their level is <= the currently configured log level.
     */
    typedef enum
    {
        LOG_LEVEL_NONE = 0,
        LOG_LEVEL_CRITICAL = 1,
        LOG_LEVEL_ERROR = 2,
        LOG_LEVEL_WARNING = 3,
        LOG_LEVEL_DEBUG = 4
    } log_level_t;

    /**
     * @brief Set global log level
     *
     * Controls which messages are printed. Only messages with severity
     * level <= the configured level will be output.
     *
     * @param level New log level
     */
    void set_log_level(log_level_t level);

    /**
     * @brief Get current log level
     * @return Current log level
     */
    log_level_t get_log_level(void);

    /**
     * @brief Validate if integer is valid log level
     * @param level Integer to validate
     * @return true if valid log level, false otherwise
     */
    bool validate_log_level(int level);

    /**
     * @brief Print formatted log message
     *
     * Internal function used by logging macros. Prints message only if
     * the specified level is enabled.
     *
     * @param level Message severity level
     * @param fmt Printf-style format string
     * @param ... Variable arguments for format string
     */
    void log_printf(log_level_t level, const char *fmt, ...);

/** @defgroup Log_Macros Logging Macros
 * @brief Convenient macros for logging at different severity levels
 * @{
 */

/** Log critical error message */
#define LOG_CRITICAL(fmt, ...) log_printf(LOG_LEVEL_CRITICAL, fmt, ##__VA_ARGS__)

/** Log error message */
#define LOG_ERROR(fmt, ...) log_printf(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/** Log warning message */
#define LOG_WARN(fmt, ...) log_printf(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)

/** Log debug message */
#define LOG_DEBUG(fmt, ...) log_printf(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
