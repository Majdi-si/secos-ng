/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
#include <segmem.h>
#include <io.h>
#include <cr.h>
#include "include/config.h"
#include "include/handlers.h"
#include "include/tss.h"

/* ============================================
 * Gestionnaire d'appel système (int 0x80)
 * ============================================ */

void syscall_isr(void) {
    asm volatile (
        "leave               \n"
        "pusha               \n"
        "mov %%esp, %%eax    \n"
        "call syscall_handler\n"
        "popa                \n"
        "iret"
        ::: "memory"
    );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx) {
    uint32_t syscall_num = ctx->gpr.eax.raw;
    
    if (syscall_num == 1) {
        // Syscall 1 = fin de tâche
        debug("\n========================================\n");
        debug("=== TACHE TERMINEE AVEC SUCCES ! ===\n");
        debug("========================================\n");
        debug("Le compteur a atteint 50.\n");
        debug("Systeme multi-taches operationnel.\n\n");
        
        // Arrêter proprement
        while(1) {
            asm volatile("hlt");
        }
    } else {
        // Syscall 0 = afficher compteur
        uint32_t* counter = (uint32_t*)ctx->gpr.esi.raw;
        
        if (counter != NULL) {
            uint32_t val = *counter;
            debug("[SYSCALL] Counter = %d", val);
            
            // Messages de progression
            if (val == 1) {
                debug(" <- Debut du comptage");
            } else if (val == 10) {
                debug(" <- 20%% complete");
            } else if (val == 25) {
                debug(" <- 50%% complete");
            } else if (val == 40) {
                debug(" <- 80%% complete");
            } else if (val == 50) {
                debug(" <- 100%% complete !");
            }
            debug("\n");
        }
    }
}

/* ============================================
 * Configuration IDT
 * ============================================ */

void setup_idt(void) {
    idt_reg_t idtr;
    get_idtr(idtr);
    
    int_desc_t* dsc = &idtr.desc[0x80];
    
    // Installer le handler syscall (int 0x80)
    dsc->offset_1 = (uint16_t)((uint32_t)syscall_isr);
    dsc->offset_2 = (uint16_t)(((uint32_t)syscall_isr) >> 16);
    dsc->dpl = 3;  // Accessible depuis ring 3
    
    debug("IDT configuree (syscall int 0x80)\n");
}
