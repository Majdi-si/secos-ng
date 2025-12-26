/* GPLv2 (c) Airbus */
#ifndef __TSS_H__
#define __TSS_H__

#include <types.h>
#include <segmem.h>

/* ============================================
 * TSS (Task State Segment)
 * Utilise la structure tss_t du kernel
 * ============================================ */

/* Fonctions */
void init_tss(void);
tss_t* get_tss(void);

#endif /* __TSS_H__ */
