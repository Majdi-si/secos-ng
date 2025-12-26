/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

/* Q1
 * Dans le linker, il y a :
 * . = 0x300000;
 * .mbh      : { KEEP(*(.mbh)) . = ALIGN(4);     } : phboot
 * .stack    : { KEEP(*(.stack))                 } : phstack
 * 
 * __kernel_start__ = .;
 *
 * Et dans le entry.s, la stack fait : 
 * .section .stack, "aw", @nobits
 * .align 16
 * .space 0x2000
 * 
 * donc le symbole __kernel_start__ est situé à :
 * @__kernel_start__ = @base + al_size_mbh + size_stack 
 * = 0x300000 + aligned(4) + aligned(16) + 0x2000 
 * = 0x302010
 */

// Q2 : 
char* inttypetochar(int t) {
   switch(t) {

      case 1:
         return("MULTIBOOT_MEMORY_AVAILABLE");
      case 2:
         return("MULTIBOOT_MEMORY_RESERVED");
      case 3:
         return("MULTIBOOT_MEMORY_ACPI_RECLAIMABLE");
      case 4:
         return("MULTIBOOT_MEMORY_NVS");
      default:
         return("Unknown type");
   }
}


void tp() {
   debug("kernel mem [0x%p - 0x%p]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);

   multiboot_memory_map_t* entry = (multiboot_memory_map_t*)info->mbi->mmap_addr;
   while((uint32_t)entry < (info->mbi->mmap_addr + info->mbi->mmap_length)) {
      // Q2 
      debug("[0x%x - ", (unsigned int)entry->addr);
      debug("0x%x]", (unsigned int) (entry->len + entry->addr - 1));
      debug(" %s\n", inttypetochar(entry->type));
      // end Q2
      entry++;
   }
      
   // Q3
   int *ptr_in_reserved_mem;
   ptr_in_reserved_mem = (int*)0xf0000; // from 0xC0000 to 0xfffff: reserved for ROM mapping 
   debug("Reserved mem (at: 0xf0000):  before: 0x%x ", *ptr_in_reserved_mem); // read
   *ptr_in_reserved_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_in_reserved_mem);                // check

   int *ptr_in_available_mem;
   ptr_in_available_mem = (int*)0x0;
   debug("Available mem (0x0): before: 0x%x ", *ptr_in_available_mem); // read
   *ptr_in_available_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_in_available_mem);                // check

   // res Q3
   // Reserved mem:  before: 0x0 after: 0x0
   // Available mem: before: 0x0 after: 0x1234
   // end Q3

   // Q4
   int* ptr_out_of_mem = (int*)0x8000000;
   debug("Writing mem > 128 MB(0x100000): before: 0x%x ", *ptr_out_of_mem); // read
   *ptr_out_of_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_out_of_mem);                // check
   // Aucune erreur, ça doit être qu'on est déjà en mode protégé et qu'une GDT
   // est déjà en place :)
   // end Q4
 

}
