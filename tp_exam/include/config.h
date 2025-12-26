/* GPLv2 (c) Airbus */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <segmem.h>

/* ============================================
 * Configuration mémoire du système
 * ============================================ */

// Pagination
#define PGD_ADDR        0x600000
#define PTB_ADDR        0x601000

// Piles utilisateur (4KB chacune)
#define USER_STACK_T1   0x700000
#define USER_STACK_T2   0x800000

// Zone mémoire partagée
#define SHARED_PHYS     0x650000
#define SHARED_VIRT_T1  0x700000
#define SHARED_VIRT_T2  0x800000

// Sélecteurs GDT
#define GDT_CODE_R0_IDX 1
#define GDT_DATA_R0_IDX 2
#define GDT_CODE_R3_IDX 3
#define GDT_DATA_R3_IDX 4
#define GDT_TSS_IDX     5
#define GDT_SIZE        6

// Macros pour les sélecteurs
#define SEL_CODE_R0     gdt_krn_seg_sel(GDT_CODE_R0_IDX)
#define SEL_DATA_R0     gdt_krn_seg_sel(GDT_DATA_R0_IDX)
#define SEL_CODE_R3     gdt_usr_seg_sel(GDT_CODE_R3_IDX)
#define SEL_DATA_R3     gdt_usr_seg_sel(GDT_DATA_R3_IDX)
#define SEL_TSS         gdt_krn_seg_sel(GDT_TSS_IDX)

// Taille des piles
#define STACK_SIZE      0x1000

// Nombre de tâches
#define NB_TASKS        2

#endif /* __CONFIG_H__ */
