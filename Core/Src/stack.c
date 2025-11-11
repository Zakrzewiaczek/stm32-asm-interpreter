#include <stddef.h>
#include "stack.h"
#include "memory.h"
#include "cpu.h"
#include "log.h"

void stack_init(void)
{
    cpu.R[CPU_SP] = STACK_TOP;
    LOG_DEBUG("Stack pointer initialized: SP=0x%08lX", (uint32_t)STACK_TOP);
}

result_t stack_push(uint32_t value)
{
    uint32_t sp = cpu.R[CPU_SP];

    if (sp - 4 < STACK_BOTTOM)
    {
        LOG_ERROR("Stack overflow! SP=0x%08lX, STACK_BOTTOM=0x%08lX", (uint32_t)sp, (uint32_t)STACK_BOTTOM);
        RAISE_ERR(ERR_STACK_OVERFLOW, sp);
    }

    sp -= 4;
    result_t res = mem_write32(sp, value);

    if (is_error(res))
    {
        LOG_ERROR("Stack push failed at 0x%08lX", (uint32_t)sp);
        return res;
    }

    // Update SP register
    cpu.R[CPU_SP] = sp;
    LOG_DEBUG("PUSH: SP=0x%08lX <- 0x%08lX", (uint32_t)sp, (uint32_t)value);

    return OK;
}

result_t stack_pop(uint32_t *out_value)
{
    if (!out_value)
        RAISE_ERR(ERR_NULL_POINTER, 0);

    uint32_t sp = cpu.R[CPU_SP];

    if (sp >= STACK_TOP)
    {
        LOG_ERROR("Stack underflow! SP=0x%08lX, STACK_TOP=0x%08lX", (uint32_t)sp, (uint32_t)STACK_TOP);
        RAISE_ERR(ERR_STACK_UNDERFLOW, sp);
    }

    result_t res = mem_read32(sp, out_value);

    if (is_error(res))
    {
        LOG_ERROR("Stack pop failed at 0x%08lX", (uint32_t)sp);
        return res;
    }

    // Update SP register
    cpu.R[CPU_SP] = sp + 4;
    LOG_DEBUG("POP: SP=0x%08lX -> 0x%08lX", (uint32_t)sp, (uint32_t)*out_value);

    return OK;
}

uint32_t stack_get_sp(void)
{
    return cpu.R[CPU_SP];
}

uint32_t stack_get_depth(void)
{
    return STACK_TOP - cpu.R[CPU_SP];
}

bool stack_is_valid_address(uint32_t addr)
{
    return (addr >= STACK_BOTTOM && addr < STACK_TOP);
}
