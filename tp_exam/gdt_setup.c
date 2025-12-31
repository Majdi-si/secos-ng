/* ============================================
 * gdt_setup.c - Configuration de la GDT
 * 
 * Segments flat (base=0, limit=4GB) :
 * - Code/Data ring 0 (kernel)
 * - Code/Data ring 3 (user)
 * - TSS pour transitions ring 3 -> ring 0
 * ============================================ */

#include <debug.h>
#include <segmem.h>
#include <string.h>
#include "include/config.h"
#include "include/gdt_setup.h"
#include "include/tss.h"

seg_desc_t GDT[GDT_SIZE];

#define gdt_flat_dsc(_dSc_,_pVl_,_tYp_)                                 \
   ({                                                                   \
      (_dSc_)->raw     = 0;                                             \
      (_dSc_)->limit_1 = 0xffff;                                        \
      (_dSc_)->limit_2 = 0xf;                                           \
      (_dSc_)->type    = _tYp_;                                         \
      (_dSc_)->dpl     = _pVl_;                                         \
      (_dSc_)->d       = 1;                                             \
      (_dSc_)->g       = 1;                                             \
      (_dSc_)->s       = 1;                                             \
      (_dSc_)->p       = 1;                                             \
   })

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

seg_desc_t* get_gdt(void) {
    return GDT;
}

void init_gdt(void) {
    gdt_reg_t gdtr;
    
    GDT[0].raw = 0ULL;
    gdt_flat_dsc(&GDT[GDT_CODE_R0_IDX], 0, SEG_DESC_CODE_XR);
    gdt_flat_dsc(&GDT[GDT_DATA_R0_IDX], 0, SEG_DESC_DATA_RW);
    gdt_flat_dsc(&GDT[GDT_CODE_R3_IDX], 3, SEG_DESC_CODE_XR);
    gdt_flat_dsc(&GDT[GDT_DATA_R3_IDX], 3, SEG_DESC_DATA_RW);
    tss_dsc(&GDT[GDT_TSS_IDX], (uint32_t)get_tss());
    
    // Charger la GDT
    gdtr.desc  = GDT;
    gdtr.limit = sizeof(GDT) - 1;
    set_gdtr(gdtr);
    
    // Recharger les sélecteurs de segment
    set_cs(SEL_CODE_R0);
    
    set_ss(SEL_DATA_R0);
    set_ds(SEL_DATA_R0);
    set_es(SEL_DATA_R0);
    set_fs(SEL_DATA_R0);
    set_gs(SEL_DATA_R0);
    
    debug("GDT initialisee\n");
}
