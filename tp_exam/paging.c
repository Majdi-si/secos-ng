/* GPLv2 (c) Airbus */
#include <debug.h>
#include <pagemem.h>
#include <cr.h>
#include <string.h>
#include "include/config.h"
#include "include/paging.h"

/* ============================================
 * Fonction principale
 * ============================================ */

void init_pagination(void) {
    pde32_t* pgd = (pde32_t*)PGD_ADDR;
    pte32_t* ptb = (pte32_t*)PTB_ADDR;
    
    // Identity mapping des 4 premiers MB avec accès utilisateur
    // (nécessaire car le code user est dans le kernel)
    for(int i = 0; i < 1024; i++) {
        pg_set_entry(&ptb[i], PG_USR|PG_RW, i);
    }
    
    // Initialiser le PGD
    memset((void*)pgd, 0, PAGE_SIZE);
    pg_set_entry(&pgd[0], PG_USR|PG_RW, page_get_nr(ptb));
    
    // Mapper aussi les 4MB suivants (4-8MB) pour avoir plus d'espace
    pte32_t* ptb2 = (pte32_t*)(PTB_ADDR + PAGE_SIZE);
    for(int i = 0; i < 1024; i++) {
        pg_set_entry(&ptb2[i], PG_USR|PG_RW, i + 1024);
    }
    pg_set_entry(&pgd[1], PG_USR|PG_RW, page_get_nr(ptb2));
    
    // Pour 0x800000, on a besoin d'une nouvelle PTB (PGD[2])
    pte32_t* ptb3 = (pte32_t*)(PTB_ADDR + 2*PAGE_SIZE);
    memset((void*)ptb3, 0, PAGE_SIZE);
    int ptb_idx = pt32_get_idx((uint32_t*)SHARED_VIRT_T2);
    pg_set_entry(&ptb3[ptb_idx], PG_USR|PG_RW, page_get_nr((void*)SHARED_PHYS));
    pg_set_entry(&pgd[2], PG_USR|PG_RW, page_get_nr(ptb3));
    
    // Charger le PGD et activer la pagination
    set_cr3((uint32_t)pgd);
    
    uint32_t cr0 = get_cr0();
    set_cr0(cr0 | CR0_PG);
    
    debug("Pagination activee\n");
}
