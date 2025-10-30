/**
 * @file cpu.c
 * @brief ARM Cortex-M4 CPU state implementation
 */

#include "cpu.h"

/**
 * @brief Global CPU instance
 *
 * Initialized with all registers set to 0 and flags cleared.
 */
cpu_t cpu = {.R = {0}, .flags = 0};