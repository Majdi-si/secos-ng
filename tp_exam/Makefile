#!/usr/bin/make -f

include ../utils/config.mk

# Liste des fichiers objets
objects += tp.o
objects += gdt_setup.o
objects += tss.o
objects += paging.o
objects += handlers.o
objects += task.o
objects += user.o

# Include local pour les headers du tp_exam
CFLAGS += -I.

# Force le chemin vers libgcc 32-bit
CCLIB := $(shell find /usr/lib/gcc -name "libgcc.a" -path "*/32/*" 2>/dev/null | head -1)
ifeq ($(CCLIB),)
CCLIB := $(shell $(CC) -m32 -print-libgcc-file-name)
endif

include ../utils/rules.mk
