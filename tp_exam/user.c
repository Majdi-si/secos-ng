/* GPLv2 (c) Airbus */
#include <types.h>
#include "include/config.h"
#include "include/user.h"

/* ============================================
 * Fonction d'appel système
 * void sys_counter(uint32_t *counter);
 * ============================================ */

static inline void sys_counter(uint32_t* counter) {
    asm volatile(
        "mov %0, %%esi \n"
        "int $0x80"
        :: "r"(counter)
        : "esi"
    );
}

/* ============================================
 * Tâche 1 : Écrit le compteur dans la mémoire partagée
 * ============================================ */

void __attribute__((section(".user"))) user1(void) {
    volatile uint32_t* counter = (volatile uint32_t*)SHARED_VIRT_T1;
    *counter = 0;
    
    while(1) {
        (*counter)++;
        
        // Petite attente
        for (volatile int i = 0; i < 300000; i++);
    }
}

/* ============================================
 * Tâche 2 : Lit et affiche le compteur via syscall
 * ============================================ */

void __attribute__((section(".user"))) user2(void) {
    volatile uint32_t* counter = (volatile uint32_t*)SHARED_VIRT_T2;
    
    while(1) {
        // Appel système pour afficher le compteur
        sys_counter((uint32_t*)counter);
        
        // Petite attente
        for (volatile int i = 0; i < 200000; i++);
    }
}
