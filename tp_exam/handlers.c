/* ============================================
 * handlers.c - Gestionnaires d'interruptions
 * 
 * - Syscall (int 0x80) : affichage compteur
 * - Timer (int 32) : ordonnancement preemptif
 * - Page Fault (int 14) : debug
 * ============================================ */

#include <debug.h>
#include <intr.h>
#include <segmem.h>
#include <io.h>
#include <cr.h>
#include <pic.h>
#include "include/config.h"
#include "include/handlers.h"
#include "include/task.h"
#include "include/tss.h"

/* ============================================
 * Gestionnaire d'appel système (int 0x80)
 * 
 * Interface: void sys_counter(uint32_t *counter);
 * Le pointeur est passé via ESI
 * ============================================ */

void syscall_isr(void) {
    asm volatile (
        "leave               \n"
        "pusha               \n"
        "mov %%esp, %%eax    \n"
        "call syscall_handler\n"
        "popa                \n"
        "iret"
        ::: "memory"
    );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx) {
    uint32_t* counter = (uint32_t*)ctx->gpr.esi.raw;
    if (counter != NULL) {
        int task = get_current_task();
        debug("[Task%d SYSCALL] Counter = %d\n", task + 1, *counter);
    }
}

/* ============================================
 * Gestionnaire Page Fault (int 14) - DEBUG
 * ============================================ */

void pagefault_isr(void) {
    asm volatile (
        "leave               \n"
        "pusha               \n"
        "mov %%esp, %%eax    \n"
        "call pagefault_handler\n"
        "popa                \n"
        "add $4, %%esp       \n"
        "iret"
        ::: "memory"
    );
}

void __regparm__(1) pagefault_handler(int_ctx_t *ctx) {
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
    debug("!!! PAGE FAULT at 0x%x, EIP=0x%x, CS=0x%x !!!\n", 
          fault_addr, ctx->eip.raw, ctx->cs.raw);
    while(1);  // Halt
}

/* ============================================
 * Gestionnaire timer (IRQ0 = int 32)
 * Ordonnancement préemptif
 * ============================================ */

void timer_isr(void) {
    asm volatile (
        "leave               \n"
        "pusha               \n"
        "mov %%esp, %%eax    \n"
        "call timer_handler  \n"
        "popa                \n"
        "iret"
        ::: "memory"
    );
}

void __regparm__(1) timer_handler(int_ctx_t *ctx) {
    outb(0x20, 0x20);  // EOI
    
    static int tick = 0;
    tick++;
    
    int current = get_current_task();
    debug("[TIMER %d] Task%d CS=0x%x\n", tick, current+1, ctx->cs.raw);
    
    if ((ctx->cs.raw & 3) == 0)
        return;
    
    task_t* tasks = get_tasks();
    tasks[current].esp = (uint32_t)ctx;
    
    int next = (current + 1) % NB_TASKS;
    if (!tasks[next].active)
        return;
    
    set_current_task(next);
    
    tss_t* tss = get_tss();
    tss->s0.esp = tasks[next].kstack + STACK_SIZE;
    set_cr3(tasks[next].cr3);
    
    if (tasks[next].esp == 0) {
        uint32_t ustack = tasks[next].ustack + STACK_SIZE - 4;
        uint32_t kstack = tasks[next].kstack + STACK_SIZE - 4;
        uint32_t eflags = 0x202;
        
        asm volatile (
            "mov %0, %%esp    \n"
            "push %1          \n"
            "push %2          \n"
            "push %3          \n"
            "push %4          \n"
            "push %5          \n"
            "iret"
            ::
            "r"(kstack),
            "i"(SEL_DATA_R3),
            "m"(ustack),
            "r"(eflags),
            "i"(SEL_CODE_R3),
            "r"(tasks[next].entry)
            : "memory"
        );
    } else {
        asm volatile (
            "mov %0, %%esp    \n"
            "popa             \n"
            "iret"
            :: "r"(tasks[next].esp)
            : "memory"
        );
    }
}

/* ============================================
 * Configuration IDT
 * ============================================ */

void setup_idt(void) {
    idt_reg_t idtr;
    get_idtr(idtr);
    int_desc_t* dsc;
    
    // Page fault (int 14)
    dsc = &idtr.desc[14];
    dsc->offset_1 = (uint16_t)((uint32_t)pagefault_isr);
    dsc->offset_2 = (uint16_t)(((uint32_t)pagefault_isr) >> 16);
    dsc->dpl = 0;
    
    // Syscall (int 0x80)
    dsc = &idtr.desc[0x80];
    dsc->offset_1 = (uint16_t)((uint32_t)syscall_isr);
    dsc->offset_2 = (uint16_t)(((uint32_t)syscall_isr) >> 16);
    dsc->dpl = 3;
    
    // Timer IRQ0 (int 32)
    dsc = &idtr.desc[32];
    dsc->offset_1 = (uint16_t)((uint32_t)timer_isr);
    dsc->offset_2 = (uint16_t)(((uint32_t)timer_isr) >> 16);
    dsc->dpl = 0;
    
    // Demasquer IRQ0
    uint8_t mask = inb(PIC1 + 1);
    outb(mask & 0xFE, PIC1 + 1);
    
    debug("IDT configuree (syscall 0x80 + timer IRQ0)\n");
}
