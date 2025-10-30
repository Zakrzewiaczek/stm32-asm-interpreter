#ifndef __REGISTERS_MAP_H
#define __REGISTERS_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define REGISTERS_COUNT 1102

typedef struct {
    const char* name;
    volatile void* address;
    size_t size;
} register_t;

extern register_t hwregs[REGISTERS_COUNT];

register_t* get_register(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* __REGISTERS_MAP_H */