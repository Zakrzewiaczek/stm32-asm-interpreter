/**
 * @file    cpu.h
 * @brief   ARM Cortex-M4 CPU state and APSR flags
 * @author  Jakub Zakrzewski
 * @date    2025
 *
 * Provides the virtual CPU register file (R0-R15) and Application Program
 * Status Register (APSR) with N, Z, C, V condition flags. This module
 * emulates the ARM Cortex-M4 register set for the interpreter.
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @defgroup APSR_Flags Application Program Status Register Flags
     * @brief ARM APSR condition code flags (NZCV)
     * @{
     */

#define CPU_FLAG_N_Pos 31 ///< Negative flag bit position
#define CPU_FLAG_Z_Pos 30 ///< Zero flag bit position
#define CPU_FLAG_C_Pos 29 ///< Carry flag bit position
#define CPU_FLAG_V_Pos 28 ///< Overflow flag bit position

#define CPU_FLAG_N_Msk (1UL << CPU_FLAG_N_Pos) ///< Negative flag mask
#define CPU_FLAG_Z_Msk (1UL << CPU_FLAG_Z_Pos) ///< Zero flag mask
#define CPU_FLAG_C_Msk (1UL << CPU_FLAG_C_Pos) ///< Carry flag mask
#define CPU_FLAG_V_Msk (1UL << CPU_FLAG_V_Pos) ///< Overflow flag mask

/** @brief Negative flag value */
#define CPU_FLAG_N ((cpu.flags & CPU_FLAG_N_Msk) >> CPU_FLAG_N_Pos)

/** @brief Zero flag value */
#define CPU_FLAG_Z ((cpu.flags & CPU_FLAG_Z_Msk) >> CPU_FLAG_Z_Pos)

/** @brief Carry flag value */
#define CPU_FLAG_C ((cpu.flags & CPU_FLAG_C_Msk) >> CPU_FLAG_C_Pos)

/** @brief Overflow flag value */
#define CPU_FLAG_V ((cpu.flags & CPU_FLAG_V_Msk) >> CPU_FLAG_V_Pos)

    /** @} */

    /** @defgroup Register_Aliases ARM Register Aliases
     * @brief Standard ARM register name mappings
     * @{
     */

#define CPU_SP 13 ///< Stack Pointer (R13)

    /** @} */

    /**
     * @brief CPU state structure
     *
     * Represents the complete state of the virtual ARM CPU including
     * general-purpose registers (R0-R15) and APSR flags.
     */
    typedef struct
    {
        uint32_t R[16]; ///< General-purpose registers (R0-R15)
        uint32_t flags; ///< APSR flags register
    } cpu_t;

    /**
     * @brief Global CPU state instance
     *
     * Single instance of the virtual CPU state. All instruction handlers
     * read from and write to this structure.
     */
    extern cpu_t cpu;

#ifdef __cplusplus
}
#endif

#endif /* CPU_H */
