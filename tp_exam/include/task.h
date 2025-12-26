/* GPLv2 (c) Airbus */
#ifndef __TASK_H__
#define __TASK_H__

#include <types.h>

/* ============================================
 * Structure de contexte de tâche
 * ============================================ */

typedef struct task_context {
    uint32_t esp0;          // Pointeur pile noyau
    uint32_t cr3;           // PGD de la tâche
    uint32_t active;        // Tâche active ?
} task_t;

/* Fonctions */
void init_tasks(void);
void start_first_task(void);
task_t* get_tasks(void);
int get_current_task(void);
void set_current_task(int task_id);

#endif /* __TASK_H__ */
