# Documentation - Cartographie Mémoire
## TP Exam SecOS - SI SALAH Majdi

---

## 1. Vue d'ensemble

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CARTOGRAPHIE MÉMOIRE PHYSIQUE                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  0x0000000 ┌──────────────────────────────────────┐                         │
│            │  Réservé (BIOS, IVT, etc.)           │                         │
│  0x0300000 ├──────────────────────────────────────┤                         │
│            │  Kernel code/data (.text, .data)     │                         │
│  0x0400000 ├──────────────────────────────────────┤ ◄── 4 MB                │
│            │  PGD (Page Global Directory)         │ 4KB                     │
│  0x0401000 ├──────────────────────────────────────┤                         │
│            │  PTB[0] - Identity map 0-4MB         │ 4KB                     │
│  0x0402000 ├──────────────────────────────────────┤                         │
│            │  PTB[1] - Identity map 4-8MB         │ 4KB                     │
│  0x0403000 ├──────────────────────────────────────┤                         │
│            │  PTB[2] - Identity map 8-12MB        │ 4KB                     │
│  0x0404000 ├──────────────────────────────────────┤                         │
│            │  PTB_SHR_T1 - Shared mem Task1       │ 4KB                     │
│  0x0405000 ├──────────────────────────────────────┤                         │
│            │  PTB_SHR_T2 - Shared mem Task2       │ 4KB                     │
│  0x0500000 ├──────────────────────────────────────┤ ◄── 5 MB                │
│            │  MÉMOIRE PARTAGÉE (physique)         │ 4KB                     │
│  0x0600000 ├──────────────────────────────────────┤ ◄── 6 MB                │
│            │  Pile Noyau Task 1                   │ 4KB                     │
│  0x0601000 ├──────────────────────────────────────┤                         │
│            │  Pile Noyau Task 2                   │ 4KB                     │
│  0x0700000 ├──────────────────────────────────────┤ ◄── 7 MB                │
│            │  Pile Utilisateur Task 1             │ 4KB                     │
│  0x0701000 ├──────────────────────────────────────┤                         │
│            │  Pile Utilisateur Task 2             │ 4KB                     │
│            └──────────────────────────────────────┘                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Structure de Pagination

### 2.1 Page Global Directory (PGD) - Unique et Partagé

| Index PGD | Adresse Virtuelle | PTB Utilisée | Description |
|-----------|-------------------|--------------|-------------|
| 0 | 0x000000 - 0x3FFFFF | PTB_0 (0x401000) | Identity mapping 0-4MB |
| 1 | 0x400000 - 0x7FFFFF | PTB_1 (0x402000) | Identity mapping 4-8MB |
| 2 | 0x800000 - 0xBFFFFF | PTB_2 (0x403000) | Identity mapping 8-12MB |
| 3 | 0xC00000 - 0xFFFFFF | PTB_SHR_T1 (0x404000) | Shared mem pour Task 1 |
| 4 | 0x1000000 - 0x13FFFFF | PTB_SHR_T2 (0x405000) | Shared mem pour Task 2 |

### 2.2 Mémoire Partagée

```
                    TASK 1                          TASK 2
                      │                               │
                      ▼                               ▼
              ┌───────────────┐              ┌───────────────┐
              │ Adresse Virt. │              │ Adresse Virt. │
              │   0xC00000    │              │  0x1000000    │
              │   (12 MB)     │              │   (16 MB)     │
              └───────┬───────┘              └───────┬───────┘
                      │                              │
                      │    PGD[3]                    │      PGD[4]
                      ▼                              ▼
              ┌───────────────┐              ┌───────────────┐
              │  PTB_SHR_T1   │              │  PTB_SHR_T2   │
              │  (0x404000)   │              │  (0x405000)   │
              └───────┬───────┘              └───────┬───────┘
                      │                               │
                      └───────────────┬───────────────┘
                                      ▼
                              ┌───────────────┐
                              │ Adresse Phys. │
                              │   0x500000    │
                              │   (5 MB)      │
                              │    4 KB       │
                              └───────────────┘
```

**Principe** : Les deux tâches accèdent à la même page physique (0x500000) via des adresses virtuelles différentes, permettant la communication inter-tâches.

---

## 3. Configuration des Tâches

### 3.1 Task 1 (user1)

| Élément | Adresse | Taille |
|---------|---------|--------|
| Pile Noyau | 0x600000 - 0x600FFF | 4 KB |
| Pile Utilisateur | 0x700000 - 0x700FFF | 4 KB |
| Shared Memory (virt) | 0xC00000 | 4 KB |
| CR3 (PGD) | 0x400000 | - |

### 3.2 Task 2 (user2)

| Élément | Adresse | Taille |
|---------|---------|--------|
| Pile Noyau | 0x601000 - 0x601FFF | 4 KB |
| Pile Utilisateur | 0x701000 - 0x701FFF | 4 KB |
| Shared Memory (virt) | 0x1000000 | 4 KB |
| CR3 (PGD) | 0x400000 | - |

---

## 4. Segmentation (GDT)

| Index | Sélecteur | Type | DPL | Base | Limite | Description |
|-------|-----------|------|-----|------|--------|-------------|
| 0 | 0x00 | Null | - | - | - | Descripteur nul |
| 1 | 0x08 | Code | 0 | 0x0 | 4GB | Code Kernel |
| 2 | 0x10 | Data | 0 | 0x0 | 4GB | Data Kernel |
| 3 | 0x1B | Code | 3 | 0x0 | 4GB | Code User (ring 3) |
| 4 | 0x23 | Data | 3 | 0x0 | 4GB | Data User (ring 3) |
| 5 | 0x28 | TSS | 0 | &tss | 104 | Task State Segment |

**Modèle** : Flat memory model (base = 0, limite = 4GB pour tous les segments)

---

## 5. Interruptions (IDT)

| Vecteur | Type | DPL | Handler | Description |
|---------|------|-----|---------|-------------|
| 14 | Trap | 0 | `pagefault_isr` | Page Fault (debug) |
| 32 | Interrupt | 0 | `timer_isr` | IRQ0 - Timer (préemption) |
| 0x80 | Trap | 3 | `syscall_isr` | Appel système |

---

## 6. Flux d'Exécution

```
┌─────────────────────────────────────────────────────────────────────┐
│                      ORDONNANCEMENT PRÉEMPTIF                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│    ┌──────────┐         IRQ0          ┌──────────┐                  │
│    │  TASK 1  │ ◄────────────────────►│  TASK 2  │                  │
│    │  Ring 3  │      timer_isr        │  Ring 3  │                  │
│    │          │                       │          │                  │
│    │ Écrit    │                       │ Lit et   │                  │
│    │ compteur │                       │ affiche  │                  │
│    │ @0xC00000│                       │@0x1000000│                  │
│    └──────────┘                       └──────────┘                  │
│          │                                  │                       │
│          │                                  │                       │
│          │         ┌──────────────┐         │                       │
│          └────────►│ Mém. Partagée│◄────────┘                       │
│                    │  @ 0x500000  │                                 │
│                    │  (physique)  │                                 │
│                    └──────────────┘                                 │
│                                                                     │
│    Syscall int 0x80 : sys_counter(uint32_t *counter)                │
│    → Affiche la valeur du compteur via debug()                      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 7. Vérification des Exigences

| Exigence | Status | Implémentation |
|----------|--------|----------------|
| 2 tâches en ring 3 | ✅ | `user1()` et `user2()` avec `__attribute__((section(".user")))` |
| Boucles infinies | ✅ | `while(1)` dans chaque tâche |
| Pagination activée | ✅ | `init_pagination()` avec CR0.PG=1 |
| Identity mapping kernel | ✅ | 0-12MB mappé identiquement |
| Identity mapping tâches | ✅ | Même PGD, mêmes PTB pour 0-12MB |
| PGD/PTB propres | ✅ | PGD partagé, PTB séparées pour shared mem |
| Zone mémoire partagée 4KB | ✅ | 0x500000 (phys) mappé à 2 adresses virt |
| Adresses virtuelles différentes | ✅ | T1: 0xC00000, T2: 0x1000000 |
| Pile noyau par tâche (4KB) | ✅ | T1: 0x600000, T2: 0x601000 |
| Pile user par tâche (4KB) | ✅ | T1: 0x700000, T2: 0x701000 |
| Task 1 écrit compteur | ✅ | Incrémente `*counter` en boucle |
| Task 2 affiche compteur | ✅ | Appelle `sys_counter()` |
| Syscall int 0x80 | ✅ | `syscall_isr` avec DPL=3 |
| Timer IRQ0 (int 32) | ✅ | `timer_isr` avec round-robin |
| Détection kernel/user | ✅ | Test `(ctx->cs.raw & 3) == 0` |

---

## 8. Fichiers du Projet

```
tp_exam/
├── include/
│   ├── config.h      # Configuration mémoire et sélecteurs
│   ├── gdt_setup.h   # Prototypes GDT
│   ├── handlers.h    # Prototypes handlers interruptions
│   ├── paging.h      # Prototypes pagination
│   ├── task.h        # Structure task_t et prototypes
│   ├── tss.h         # Prototypes TSS
│   └── user.h        # Prototypes fonctions user
├── gdt_setup.c       # Initialisation GDT (flat model)
├── handlers.c        # Syscall, Timer, Page Fault handlers
├── paging.c          # Configuration pagination
├── task.c            # Gestion des tâches et scheduler
├── tp.c              # Point d'entrée (tp())
├── tss.c             # Initialisation TSS
├── user.c            # Code des tâches ring 3
├── Makefile
└── Documentation.md     # Cette documentation
```

