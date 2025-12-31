#ifndef __HANDLERS_H__
#define __HANDLERS_H__

#include <intr.h>

/* Fonctions assembleur (handlers) */
void syscall_isr(void);
void timer_isr(void);
void pagefault_isr(void);

/* Fonctions C */
void __regparm__(1) syscall_handler(int_ctx_t *ctx);
void __regparm__(1) timer_handler(int_ctx_t *ctx);
void __regparm__(1) pagefault_handler(int_ctx_t *ctx);

/* Configuration IDT */
void setup_idt(void);

#endif /* __HANDLERS_H__ */
