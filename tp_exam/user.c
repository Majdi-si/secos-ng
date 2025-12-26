/* GPLv2 (c) Airbus */
#include <types.h>
#include "include/config.h"
#include "include/user.h"

/* ============================================
 * Tâche 1 : Incrémente le compteur et appelle syscall
 * ============================================ */

void __attribute__((section(".user"))) user1(void) {
    volatile uint32_t* counter = (volatile uint32_t*)SHARED_VIRT_T1;
    *counter = 0;
    
    while(*counter < 50) {
        (*counter)++;
        
        // Appel système pour afficher le compteur
        asm volatile(
            "mov %0, %%esi \n"
            "int $0x80"
            :: "r"(counter)
            : "esi"
        );
        
        // Petite attente
        for (volatile int i = 0; i < 500000; i++);
    }
    
    // Fin de la tâche - appel système spécial (eax=1 pour signaler la fin)
    asm volatile(
        "mov $1, %%eax \n"
        "int $0x80"
        ::: "eax"
    );
    
    // Boucle infinie (ne devrait pas être atteinte)
    while(1);
}

/* ============================================
 * Tâche 2 : (non utilisée pour l'instant)
 * ============================================ */

void __attribute__((section(".user"))) user2(void) {
    volatile uint32_t* counter = (volatile uint32_t*)SHARED_VIRT_T2;
    
    while(1) {
        // Appel système pour afficher le compteur
        asm volatile(
            "mov %0, %%esi \n"
            "int $0x80"
            :: "r"(counter)
            : "esi"
        );
        // Petite attente
        for (volatile int i = 0; i < 500000; i++);
    }
}
