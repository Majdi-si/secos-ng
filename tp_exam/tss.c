/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>
#include "include/config.h"
#include "include/tss.h"

/* ============================================
 * Variables globales
 * ============================================ */

tss_t TSS;

/* ============================================
 * Fonctions
 * ============================================ */

tss_t* get_tss(void) {
    return &TSS;
}

void init_tss(void) {
    memset((void*)&TSS, 0, sizeof(tss_t));
    TSS.s0.ss  = SEL_DATA_R0;
    TSS.s0.esp = get_ebp();  // Pile noyau courante
    
    // Charger le TSS
    set_tr(SEL_TSS);
    
    debug("TSS initialise\n");
}
