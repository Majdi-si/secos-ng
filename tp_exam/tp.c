#include <debug.h>
#include <intr.h>
#include <segmem.h>

#include "include/config.h"
#include "include/gdt_setup.h"
#include "include/tss.h"
#include "include/paging.h"
#include "include/handlers.h"
#include "include/task.h"

/* ============================================
 * Point d'entrée principal du TP Examen
 * 
 * OS Multi-tâches préemptif :
 * - 2 tâches en ring 3
 * - Pagination avec PGD/PTB séparés par tâche
 * - Mémoire partagée entre les tâches
 * - Ordonnancement préemptif via timer (IRQ0)
 * - Appel système pour affichage (int 0x80)
 * ============================================ */

void tp(void) {
    debug("======================================\n");
    debug("=== TP Exam: OS Multi-taches      ===\n");
    debug("======================================\n\n");
    
    debug("[1/5] Initialisation GDT...\n");
    init_gdt();
    
    debug("[2/5] Initialisation TSS...\n");
    init_tss();
    
    debug("[3/5] Initialisation Pagination...\n");
    init_pagination();
    
    debug("[4/5] Configuration IDT...\n");
    setup_idt();
    
    debug("[5/5] Initialisation Taches...\n");
    init_tasks();
    
    debug("\n======================================\n");
    debug("Task1: ecrit compteur en memoire partagee\n");
    debug("Task2: lit et affiche via syscall\n");
    debug("Preemption via timer IRQ0 (int 32)\n");
    debug("======================================\n");
    
    start_scheduler();
    
    while(1);
}
