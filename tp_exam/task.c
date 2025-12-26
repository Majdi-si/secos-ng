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
static int current_task = 0;

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
 * Initialisation d'une tâche
 * ============================================ */

static void init_task(int task_id, void (*entry)(), uint32_t pgd, 
                      uint32_t kernel_stack, uint32_t user_stack) {
    // Préparer le contexte initial sur la pile noyau
    uint32_t* kstack = (uint32_t*)(kernel_stack + STACK_SIZE);
    
    // Empiler le contexte pour IRET (transition vers ring 3)
    *(--kstack) = SEL_DATA_R3 | 3;              // SS
    *(--kstack) = user_stack + STACK_SIZE - 4;  // ESP
    *(--kstack) = 0x202;                         // EFLAGS (IF=1)
    *(--kstack) = SEL_CODE_R3 | 3;              // CS
    *(--kstack) = (uint32_t)entry;              // EIP
    
    // Error code et numéro d'interruption (factices)
    *(--kstack) = 0;  // Error code
    *(--kstack) = 0;  // Int number
    
    // Registres généraux (PUSHA order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    *(--kstack) = 0;  // EAX
    *(--kstack) = 0;  // ECX
    *(--kstack) = 0;  // EDX
    *(--kstack) = 0;  // EBX
    *(--kstack) = 0;  // ESP (ignoré par POPA)
    *(--kstack) = 0;  // EBP
    *(--kstack) = 0;  // ESI
    *(--kstack) = 0;  // EDI
    
    tasks[task_id].esp0 = (uint32_t)kstack;
    tasks[task_id].cr3 = pgd;
    tasks[task_id].active = 1;
}

/* ============================================
 * Initialisation des tâches
 * ============================================ */

void init_tasks(void) {
    // Initialiser mémoire partagée à 0
    memset((void*)SHARED_PHYS, 0, 0x1000);
    
    // Initialiser les deux tâches
    init_task(0, user1, PGD_TASK1, KERNEL_STACK_T1, USER_STACK_T1);
    init_task(1, user2, PGD_TASK2, KERNEL_STACK_T2, USER_STACK_T2);
    
    current_task = 0;
    
    debug("Taches initialisees\n");
}

/* ============================================
 * Démarrage de la première tâche
 * ============================================ */

void start_first_task(void) {
    // Configurer le TSS pour la première tâche
    tss_t* tss = get_tss();
    tss->s0.esp = KERNEL_STACK_T1 + STACK_SIZE;
    tss->s0.ss  = SEL_DATA_R0;
    
    // Charger le PGD de la première tâche
    set_cr3(tasks[0].cr3);
    
    debug("Demarrage de la premiere tache...\n");
    
    // Activer les interruptions et sauter vers la tâche
    asm volatile(
        "mov %0, %%esp          \n"
        "popa                   \n"
        "add $8, %%esp          \n"
        "sti                    \n"
        "iret"
        :: "r"(tasks[0].esp0)
    );
}
