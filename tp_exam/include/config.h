/* GPLv2 (c) Airbus */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <segmem.h>

/* ============================================
 * Configuration mémoire du système
 * ============================================
 * 
 * Cartographie mémoire physique :
 * 
 * 0x000000 - 0x300000 : Réservé (BIOS, etc.)
 * 0x300000 - 0x400000 : Kernel code/data
 * 
 * 0x400000 : PGD commun (kernel + tâches)
 * 0x401000 : PTB[0] - pages 0-4MB
 * 0x402000 : PTB[1] - pages 4-8MB
 * 0x403000 : PTB[2] - pages 8-12MB  
 * 0x404000 : PTB pour shared mem T1 (PGD[9])
 * 0x405000 : PTB pour shared mem T2 (PGD[10])
 * 
 * 0x500000 : Mémoire partagée (physique)
 * 
 * 0x600000 : Pile noyau Task 1 (4KB)
 * 0x601000 : Pile noyau Task 2 (4KB)
 * 
 * 0x700000 : Pile utilisateur Task 1 (4KB)
 * 0x701000 : Pile utilisateur Task 2 (4KB)
 * 
 * Adresses virtuelles mémoire partagée :
 * - Task 1 : 0x900000 (PGD[9])
 * - Task 2 : 0xA00000 (PGD[10])
 */

// Pagination (PGD unique partagé)
#define PGD_ADDR        0x400000
#define PTB_0           0x401000
#define PTB_1           0x402000
#define PTB_2           0x403000
#define PTB_SHR_T1      0x404000
#define PTB_SHR_T2      0x405000

// Mémoire partagée - adresses virtuelles au-delà de l'identity mapping
#define SHARED_PHYS     0x500000
#define SHARED_VIRT_T1  0xC00000    // 12MB - PGD[3]
#define SHARED_VIRT_T2  0x1000000   // 16MB - PGD[4]

// Piles noyau (4KB chacune)
#define KERNEL_STACK_T1 0x600000
#define KERNEL_STACK_T2 0x601000

// Piles utilisateur (4KB chacune)
#define USER_STACK_T1   0x700000
#define USER_STACK_T2   0x701000

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
