/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <intr.h>
#include <cr.h>
#include <string.h>
#include "include/config.h"
#include "include/task.h"
#include "include/tss.h"
#include "include/user.h"

/* ============================================
 * Variables globales
 * ============================================ */

static task_t tasks[NB_TASKS];
static int current_task = -1;  // -1 = kernel, 0 = task1, 1 = task2

/* ============================================
 * Accesseurs
 * ============================================ */

task_t* get_tasks(void) {
    return tasks;
}

int get_current_task(void) {
    return current_task;
}

void set_current_task(int task_id) {
    current_task = task_id;
}

/* ============================================
 * Initialisation des tâches
 * ============================================ */

void init_tasks(void) {
    // Task 1 : écrit dans la mémoire partagée
    tasks[0].cr3    = PGD_ADDR;  // Même PGD pour toutes les tâches
    tasks[0].kstack = KERNEL_STACK_T1;
    tasks[0].ustack = USER_STACK_T1;
    tasks[0].entry  = user1;
    tasks[0].esp    = 0;
    tasks[0].active = 1;
    
    // Task 2 : lit et affiche via syscall
    tasks[1].cr3    = PGD_ADDR;  // Même PGD pour toutes les tâches
    tasks[1].kstack = KERNEL_STACK_T2;
    tasks[1].ustack = USER_STACK_T2;
    tasks[1].entry  = user2;
    tasks[1].esp    = 0;
    tasks[1].active = 1;
    
    debug("2 taches initialisees\n");
    debug("  Task1: cr3=0x%x kstack=0x%x ustack=0x%x\n", 
          tasks[0].cr3, tasks[0].kstack, tasks[0].ustack);
    debug("  Task2: cr3=0x%x kstack=0x%x ustack=0x%x\n", 
          tasks[1].cr3, tasks[1].kstack, tasks[1].ustack);
}

/* ============================================
 * Démarrage de l'ordonnanceur
 * ============================================ */

void start_scheduler(void) {
    current_task = 0;
    
    // Configurer le TSS pour la pile noyau de la tâche 0
    tss_t* tss = get_tss();
    tss->s0.esp = tasks[0].kstack + STACK_SIZE;
    tss->s0.ss  = gdt_krn_seg_sel(GDT_DATA_R0_IDX);
    
    // Charger le PGD de la tâche 0
    set_cr3(tasks[0].cr3);
    
    debug("\n=== Demarrage ordonnanceur ===\n");
    debug("Lancement Task1 (ecriture compteur)\n\n");
    
    // Passer en ring 3 avec la première tâche
    uint32_t ustack = tasks[0].ustack + STACK_SIZE - 4;
    uint32_t eflags = (1 << 9);  // IF=1 pour activer les interruptions
    uint32_t ss3 = gdt_usr_seg_sel(GDT_DATA_R3_IDX);
    uint32_t cs3 = gdt_usr_seg_sel(GDT_CODE_R3_IDX);
    
    debug("IRET: SS=0x%x ESP=0x%x EFLAGS=0x%x CS=0x%x EIP=0x%x\n",
          ss3, ustack, eflags, cs3, (uint32_t)tasks[0].entry);
    
    asm volatile (
        "push %0      \n"  // SS
        "push %1      \n"  // ESP
        "push %2      \n"  // EFLAGS avec IF=1
        "push %3      \n"  // CS
        "push %4      \n"  // EIP
        "iret"
        ::
        "r"(ss3),
        "r"(ustack),
        "r"(eflags),
        "r"(cs3),
        "r"(tasks[0].entry)
        : "memory"
    );
}
