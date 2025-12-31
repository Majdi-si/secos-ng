/* ============================================
 * tss.c - Task State Segment
 * 
 * Configure la pile kernel (ss0, esp0) pour
 * les transitions ring 3 -> ring 0
 * ============================================ */

#include <debug.h>
#include <segmem.h>
#include <string.h>
#include "include/config.h"
#include "include/tss.h"

tss_t TSS;

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
