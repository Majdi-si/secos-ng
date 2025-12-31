#include <types.h>
#include "include/config.h"
#include "include/user.h"

static inline void sys_counter(uint32_t* counter) {
    asm volatile(
        "mov %0, %%esi \n"
        "int $0x80"
        :: "r"(counter)
        : "esi"
    );
}

void __attribute__((section(".user"))) user1(void) {
    volatile uint32_t* counter = (volatile uint32_t*)SHARED_VIRT_T1;
    *counter = 0;
    
    while(1) {
        (*counter)++;
        for (volatile int i = 0; i < 300000; i++);
    }
}

void __attribute__((section(".user"))) user2(void) {
    volatile uint32_t* counter = (volatile uint32_t*)SHARED_VIRT_T2;
    
    while(1) {
        sys_counter((uint32_t*)counter);
        for (volatile int i = 0; i < 200000; i++);
    }
}
