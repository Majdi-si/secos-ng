/* GPLv2 (c) Airbus */
#ifndef __GDT_SETUP_H__
#define __GDT_SETUP_H__

#include <segmem.h>

/* Fonctions */
void init_gdt(void);
seg_desc_t* get_gdt(void);

#endif /* __GDT_SETUP_H__ */
