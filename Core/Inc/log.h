
/**
 * @file    log.h
 * @brief   Hierarchical logging system for ARM interpreter
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Provides configurable logging with multiple severity levels:
 * CRITICAL, ERROR, WARNING, DEBUG. Log output can be controlled
 * at runtime and filtered by minimum severity level.
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
     * @brief Log severity levels (hierarchical)
     *
     * Higher numeric values include all lower levels. Setting level to
     * WARNING will show CRITICAL, ERROR, and WARNING messages.
     */
    typedef enum
    {
        LOG_LEVEL_NONE = 0,     ///< No logging output
        LOG_LEVEL_CRITICAL = 1, ///< Critical errors only (system failure)
        LOG_LEVEL_ERROR = 2,    ///< Errors and critical (recoverable errors)
        LOG_LEVEL_WARNING = 3,  ///< Warnings, errors, and critical
        LOG_LEVEL_DEBUG = 4     ///< All messages including debug info
    } log_level_t;

    /* ===================================================================
     *  Public API Functions
     * =================================================================== */

    /**
     * @brief Set global log level threshold
     *
     * Messages at or below this level will be output. Higher level
     * messages are filtered out.
     *
     * @param[in] level  New log level threshold
     */
    void set_log_level(log_level_t level);

    /**
     * @brief Get current log level threshold
     *
     * @return Current log level setting
     */
    log_level_t get_log_level(void);

    /**
     * @brief Validate log level value
     *
     * Checks if an integer value corresponds to a valid log level.
     *
     * @param[in] level  Integer log level to validate
     *
     * @retval true   Valid log level (0-4)
     * @retval false  Invalid log level
     */
    bool validate_log_level(int level);

    /**
     * @brief Core logging function with formatted output
     *
     * Outputs a formatted message if the specified level is at or below
     * the current log threshold. Uses printf-style formatting.
     *
     * @param[in] level  Message severity level
     * @param[in] fmt    Printf-style format string
     * @param[in] ...    Format arguments
     */
    void log_printf(log_level_t level, const char *fmt, ...);

/** @defgroup Log_Macros Logging Convenience Macros
 * @brief Wrapper macros for common log levels
 * @{
 */

/**
 * @brief Log critical error message
 * @param fmt    Printf-style format string
 * @param ...    Format arguments
 */
#define LOG_CRITICAL(fmt, ...) log_printf(LOG_LEVEL_CRITICAL, fmt, ##__VA_ARGS__)

/**
 * @brief Log error message
 * @param fmt    Printf-style format string
 * @param ...    Format arguments
 */
#define LOG_ERROR(fmt, ...) log_printf(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/**
 * @brief Log warning message
 * @param fmt    Printf-style format string
 * @param ...    Format arguments
 */
#define LOG_WARN(fmt, ...) log_printf(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)

/**
 * @brief Log debug message
 * @param fmt    Printf-style format string
 * @param ...    Format arguments
 */
#define LOG_DEBUG(fmt, ...) log_printf(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
