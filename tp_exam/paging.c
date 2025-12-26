/* GPLv2 (c) Airbus */
#include <debug.h>
#include <pagemem.h>
#include <cr.h>
#include <string.h>
#include "include/config.h"
#include "include/paging.h"

/* ============================================
 * Fonction principale
 * 
 * Utilise un PGD unique avec identity mapping
 * et des PTB séparées pour la mémoire partagée
 * ============================================ */

void init_pagination(void) {
    pde32_t* pgd  = (pde32_t*)PGD_ADDR;
    pte32_t* ptb0 = (pte32_t*)PTB_0;
    pte32_t* ptb1 = (pte32_t*)PTB_1;
    pte32_t* ptb2 = (pte32_t*)PTB_2;
    pte32_t* ptb_shr_t1 = (pte32_t*)PTB_SHR_T1;
    pte32_t* ptb_shr_t2 = (pte32_t*)PTB_SHR_T2;
    
    // Initialiser la mémoire partagée à 0
    memset((void*)SHARED_PHYS, 0, PAGE_SIZE);
    
    // Identity mapping des 12 premiers MB avec accès utilisateur
    for(int i = 0; i < 1024; i++) {
        pg_set_entry(&ptb0[i], PG_USR|PG_RW, i);           // 0-4MB
        pg_set_entry(&ptb1[i], PG_USR|PG_RW, i + 1024);    // 4-8MB
        pg_set_entry(&ptb2[i], PG_USR|PG_RW, i + 2048);    // 8-12MB
    }
    
    // Initialiser le PGD
    memset((void*)pgd, 0, PAGE_SIZE);
    pg_set_entry(&pgd[0], PG_USR|PG_RW, page_get_nr(ptb0));
    pg_set_entry(&pgd[1], PG_USR|PG_RW, page_get_nr(ptb1));
    pg_set_entry(&pgd[2], PG_USR|PG_RW, page_get_nr(ptb2));
    
    // Configurer mémoire partagée pour Task 1 (0xC00000 = 12MB = PGD[3])
    memset((void*)ptb_shr_t1, 0, PAGE_SIZE);
    int ptb_idx_t1 = pt32_get_idx((uint32_t*)SHARED_VIRT_T1);
    pg_set_entry(&ptb_shr_t1[ptb_idx_t1], PG_USR|PG_RW, page_get_nr((void*)SHARED_PHYS));
    pg_set_entry(&pgd[3], PG_USR|PG_RW, page_get_nr(ptb_shr_t1));
    
    // Configurer mémoire partagée pour Task 2 (0x1000000 = 16MB = PGD[4])
    memset((void*)ptb_shr_t2, 0, PAGE_SIZE);
    int ptb_idx_t2 = pt32_get_idx((uint32_t*)SHARED_VIRT_T2);
    pg_set_entry(&ptb_shr_t2[ptb_idx_t2], PG_USR|PG_RW, page_get_nr((void*)SHARED_PHYS));
    pg_set_entry(&pgd[4], PG_USR|PG_RW, page_get_nr(ptb_shr_t2));
    
    // Charger le PGD et activer la pagination
    set_cr3((uint32_t)pgd);
    
    uint32_t cr0 = get_cr0();
    set_cr0(cr0 | CR0_PG);
    
    debug("Pagination activee (identity 0-12MB + shared mem)\n");
}
