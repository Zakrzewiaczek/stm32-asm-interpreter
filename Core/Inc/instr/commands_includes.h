/**
 * @file    commands_includes.h
 * @brief   Central include file for all ARM instruction implementations
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * This file serves as a central registry for all instruction group headers.
 * Each instruction category is organized into separate modules for better
 * maintainability and clear separation of concerns.
 *
 * @note Add new instruction headers here as new instruction groups are implemented
 */

#ifndef INSTR_COMMANDS_INCLUDES_H
#define INSTR_COMMANDS_INCLUDES_H

/**
 * @defgroup DataTransfer Data Transfer Instructions
 * @brief MOV, LDR, STR and their variants
 * @{
 */
#include "data_transfer.h"
/** @} */

#endif /* INSTR_COMMANDS_INCLUDES_H */