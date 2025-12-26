/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
#include <segmem.h>

#include "include/config.h"
#include "include/gdt_setup.h"
#include "include/tss.h"
#include "include/paging.h"
#include "include/handlers.h"
#include "include/user.h"

/* ============================================
 * Point d'entrée principal du TP Examen
 * ============================================ */

void tp(void) {
    debug("======================================\n");
    debug("=== TP Exam: OS Multi-taches      ===\n");
    debug("======================================\n\n");
    
    // 1. Initialiser la GDT
    debug("[1/4] Initialisation GDT...\n");
    init_gdt();
    
    // 2. Initialiser le TSS
    debug("[2/4] Initialisation TSS...\n");
    init_tss();
    
    // 3. Initialiser la pagination
    debug("[3/4] Initialisation Pagination...\n");
    init_pagination();
    
    // 4. Configurer l'IDT (syscall)
    debug("[4/4] Configuration IDT...\n");
    setup_idt();
    
    debug("\n=== Passage en ring 3 ===\n\n");
    
    // 5. Aller en ring 3 avec user1
    uint32_t ustack = USER_STACK_T1;
    asm volatile (
        "push %0 \n"  // ss
        "push %1 \n"  // esp
        "pushf   \n"  // eflags
        "push %2 \n"  // cs
        "push %3 \n"  // eip
        "iret"
        ::
        "i"(SEL_DATA_R3),
        "m"(ustack),
        "i"(SEL_CODE_R3),
        "r"(&user1)
    );
    
    // Ne devrait jamais arriver ici
    while(1);
}
