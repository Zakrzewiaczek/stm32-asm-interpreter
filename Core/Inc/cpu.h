/**
 * @file cpu.h
 * @brief ARM Cortex-M4 CPU state representation
 *
 * This module defines the simulated CPU state including general-purpose registers
 * and APSR (Application Program Status Register) flags.
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup CPU_Flags APSR Flag Positions and Masks
 * @brief Application Program Status Register (APSR) flag definitions
 * @{
 */

/** Negative flag position (bit 31) */
#define CPU_FLAG_N_Pos 31
/** Zero flag position (bit 30) */
#define CPU_FLAG_Z_Pos 30
/** Carry flag position (bit 29) */
#define CPU_FLAG_C_Pos 29
/** Overflow flag position (bit 28) */
#define CPU_FLAG_V_Pos 28

/** Negative flag mask */
#define CPU_FLAG_N_Msk (1UL << CPU_FLAG_N_Pos)
/** Zero flag mask */
#define CPU_FLAG_Z_Msk (1UL << CPU_FLAG_Z_Pos)
/** Carry flag mask */
#define CPU_FLAG_C_Msk (1UL << CPU_FLAG_C_Pos)
/** Overflow flag mask */
#define CPU_FLAG_V_Msk (1UL << CPU_FLAG_V_Pos)

/** Extract Negative flag value (0 or 1) */
#define CPU_FLAG_N ((cpu.flags & CPU_FLAG_N_Msk) >> CPU_FLAG_N_Pos)
/** Extract Zero flag value (0 or 1) */
#define CPU_FLAG_Z ((cpu.flags & CPU_FLAG_Z_Msk) >> CPU_FLAG_Z_Pos)
/** Extract Carry flag value (0 or 1) */
#define CPU_FLAG_C ((cpu.flags & CPU_FLAG_C_Msk) >> CPU_FLAG_C_Pos)
/** Extract Overflow flag value (0 or 1) */
#define CPU_FLAG_V ((cpu.flags & CPU_FLAG_V_Msk) >> CPU_FLAG_V_Pos)

    /** @} */

    /**
     * @brief ARM Cortex-M4 CPU state structure
     *
     * Contains general-purpose registers R0-R15 and APSR flags.
     * R13 typically serves as Stack Pointer (SP), R14 as Link Register (LR),
     * and R15 as Program Counter (PC).
     */
    typedef struct
    {
        uint32_t R[16]; /**< General-purpose registers R0-R15 */
        uint32_t flags; /**< Application Program Status Register (APSR) */
    } cpu_t;

    /**
     * @brief Global CPU instance
     *
     * Represents the current state of the simulated CPU.
     * Initialized with all registers and flags set to zero.
     */
    extern cpu_t cpu;

#ifdef __cplusplus
}
#endif

#endif /* CPU_H */
