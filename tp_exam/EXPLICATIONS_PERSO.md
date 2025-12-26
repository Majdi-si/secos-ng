# Explications Détaillées - Pour Comprendre le TP

Ce fichier est pour toi, pour bien comprendre ce qu'on a fait et pourquoi.

---

## Table des Matières

1. [Vue d'ensemble : C'est quoi un OS multi-tâches ?](#1-vue-densemble)
2. [La Segmentation (GDT) : Pourquoi et Comment](#2-la-segmentation-gdt)
3. [Le TSS : La Magie du Changement de Ring](#3-le-tss)
4. [La Pagination : Mémoire Virtuelle](#4-la-pagination)
5. [Les Interruptions (IDT) : Syscall et Timer](#5-les-interruptions-idt)
6. [Le Scheduler : Comment on Change de Tâche](#6-le-scheduler)
7. [Le Flux Complet : De A à Z](#7-le-flux-complet)
8. [Les Pièges Courants](#8-les-pièges-courants)

---

## 1. Vue d'ensemble

### C'est quoi le but ?

On veut faire tourner **2 programmes utilisateur** (Task 1 et Task 2) en même temps, de manière **préemptive**. 

**Préemptif** = le CPU décide quand changer de tâche (via le timer), pas les tâches elles-mêmes.

### Les 3 grandes problématiques

1. **Isolation** : Les tâches ne doivent pas pouvoir casser le kernel ou les autres tâches
2. **Communication** : Les tâches doivent pouvoir partager des données (mémoire partagée)
3. **Préemption** : Le kernel doit pouvoir interrompre une tâche à tout moment

### Comment x86 résout ça ?

- **Rings** (anneaux de privilège) : Ring 0 = kernel (tout permis), Ring 3 = user (limité)
- **Segmentation** : Définit ce que chaque ring peut accéder
- **Pagination** : Chaque tâche voit sa propre vue de la mémoire
- **Interruptions** : Permettent de reprendre le contrôle

---

## 2. La Segmentation (GDT)

### C'est quoi ?

La GDT (Global Descriptor Table) est un tableau qui décrit les **segments mémoire**. Chaque segment a :
- Une **base** (où il commence)
- Une **limite** (sa taille)
- Des **droits** (lecture, écriture, exécution, DPL)

### Pourquoi on utilise le "Flat Model" ?

On met base=0 et limite=4GB pour tous les segments. Ça veut dire que tout le monde peut accéder à toute la mémoire (du point de vue de la segmentation).

**Pourquoi ?** Parce qu'on utilise la **pagination** pour la vraie protection. La segmentation est obligatoire en x86 32-bit, mais on la "neutralise".

### Notre GDT

```c
GDT[0] = NULL           // Obligatoire, jamais utilisé
GDT[1] = Code Ring 0    // Sélecteur 0x08 - Pour le kernel
GDT[2] = Data Ring 0    // Sélecteur 0x10 - Pour le kernel
GDT[3] = Code Ring 3    // Sélecteur 0x1B - Pour les tâches user
GDT[4] = Data Ring 3    // Sélecteur 0x23 - Pour les tâches user
GDT[5] = TSS            // Sélecteur 0x28 - Pour les transitions ring
```

### Les Sélecteurs

Un sélecteur c'est : `index * 8 + RPL`

- `0x08` = index 1, RPL 0 → Code kernel
- `0x1B` = index 3, RPL 3 → Code user (0x18 + 3 = 0x1B)
- `0x23` = index 4, RPL 3 → Data user (0x20 + 3 = 0x23)

Le **RPL** (Requested Privilege Level) doit correspondre au DPL du segment.

### Dans le code (gdt_setup.c)

```c
// Descripteur pour segment code/data (flat)
#define gdt_flat_dsc(_dsc_, _type_, _dpl_) ...

// On crée nos segments
gdt_flat_dsc(&GDT[1], SEG_DESC_CODE_XR, 0);  // Code R0
gdt_flat_dsc(&GDT[2], SEG_DESC_DATA_RW, 0);  // Data R0
gdt_flat_dsc(&GDT[3], SEG_DESC_CODE_XR, 3);  // Code R3
gdt_flat_dsc(&GDT[4], SEG_DESC_DATA_RW, 3);  // Data R3
```

---

## 3. Le TSS (Task State Segment)

### C'est quoi le problème ?

Quand on est en Ring 3 et qu'une interruption arrive, le CPU doit passer en Ring 0. Mais il a besoin d'une **pile kernel** pour sauvegarder le contexte.

**Question** : Où est cette pile kernel ? Le CPU ne peut pas deviner !

### La solution : Le TSS

Le TSS est une structure qui contient (entre autres) :
- `ss0` : Le sélecteur de pile pour Ring 0
- `esp0` : Le pointeur de pile pour Ring 0

Quand une interruption arrive depuis Ring 3 :
1. Le CPU lit le TSS
2. Il charge SS avec `tss.ss0` et ESP avec `tss.esp0`
3. Il pousse le contexte d'interruption sur cette pile
4. Il saute au handler

### Dans le code (tss.c)

```c
tss_t tss;  // Structure TSS globale

void init_tss(void) {
    memset(&tss, 0, sizeof(tss));
    
    // Pile kernel par défaut (sera changée à chaque switch de tâche)
    tss.s0.ss  = gdt_krn_seg_sel(GDT_DATA_R0_IDX);  // 0x10
    tss.s0.esp = KERNEL_STACK_T1 + STACK_SIZE;      // Sommet de la pile
    
    // On charge le TR (Task Register) avec le sélecteur du TSS
    set_tr(gdt_krn_seg_sel(GDT_TSS_IDX));  // 0x28
}
```

### Pourquoi `+ STACK_SIZE` ?

Les piles x86 grandissent **vers le bas** ! 

```
0x600000  ┌─────────────┐  ← Bas de la pile (adresse basse)
          │             │
          │   (libre)   │
          │             │
0x600FFC  │  dernière   │  ← Premières données pushées
0x601000  └─────────────┘  ← Sommet initial (ESP doit pointer ici)
```

Donc si la pile est à 0x600000 et fait 0x1000 (4KB), ESP initial = 0x601000.

---

## 4. La Pagination

### C'est quoi ?

La pagination traduit les **adresses virtuelles** (ce que voit le programme) en **adresses physiques** (la vraie RAM).

### Structure à 2 niveaux

```
Adresse Virtuelle 32 bits:
┌──────────┬──────────┬──────────────┐
│ PGD idx  │ PTB idx  │    Offset    │
│ 10 bits  │ 10 bits  │   12 bits    │
└──────────┴──────────┴──────────────┘
     │           │            │
     │           │            └── Offset dans la page (0-4095)
     │           └── Index dans la Page Table (0-1023)
     └── Index dans le Page Directory (0-1023)
```

### Traduction d'adresse

1. CR3 contient l'adresse physique du PGD
2. On lit `PGD[index_pgd]` → donne l'adresse de la PTB
3. On lit `PTB[index_ptb]` → donne l'adresse physique de la page
4. On ajoute l'offset → adresse physique finale

### Identity Mapping

**Identity mapping** = adresse virtuelle == adresse physique

On fait ça pour les 12 premiers MB parce que :
- Le kernel est chargé à des adresses physiques fixes
- C'est plus simple à gérer
- Le code user est aussi dans cette zone

### Notre configuration

```c
// PGD à 0x400000
// PTB[0] mappe 0-4MB, PTB[1] mappe 4-8MB, PTB[2] mappe 8-12MB

for(int i = 0; i < 1024; i++) {
    pg_set_entry(&ptb0[i], PG_USR|PG_RW, i);         // Page i → Frame i
    pg_set_entry(&ptb1[i], PG_USR|PG_RW, i + 1024);  // Page 1024+i → Frame 1024+i
    pg_set_entry(&ptb2[i], PG_USR|PG_RW, i + 2048);  // etc.
}
```

`pg_set_entry(&entry, flags, frame_number)` :
- `PG_USR` = accessible en Ring 3
- `PG_RW` = lecture + écriture
- `frame_number` = numéro de la page physique (adresse / 4096)

### La Mémoire Partagée : Le Truc Cool

On veut que Task 1 et Task 2 partagent une page de mémoire. Mais à des adresses virtuelles différentes !

```
Task 1 écrit à 0xC00000  ──┐
                          ├──► Page physique 0x500000
Task 2 lit à 0x1000000   ──┘
```

Comment on fait ?

```c
// Task 1 : 0xC00000 = PGD[3], PTB index 0
pg_set_entry(&ptb_shr_t1[0], PG_USR|PG_RW, 0x500000 / 4096);
pg_set_entry(&pgd[3], PG_USR|PG_RW, ptb_shr_t1 / 4096);

// Task 2 : 0x1000000 = PGD[4], PTB index 0
pg_set_entry(&ptb_shr_t2[0], PG_USR|PG_RW, 0x500000 / 4096);
pg_set_entry(&pgd[4], PG_USR|PG_RW, ptb_shr_t2 / 4096);
```

Les deux entrées pointent vers la même frame physique (0x500000 / 4096 = 0x500).

### Pourquoi un seul PGD ?

Dans une vraie implémentation, chaque tâche aurait son propre PGD. Mais ici :
- On utilise un seul PGD partagé
- Les 12 premiers MB sont identiques pour tous
- Seules les entrées pour la mémoire partagée diffèrent... mais comme les deux tâches pointent vers la même page physique, ça marche !

---

## 5. Les Interruptions (IDT)

### C'est quoi ?

L'IDT (Interrupt Descriptor Table) dit au CPU quoi faire quand une interruption arrive.

Chaque entrée contient :
- L'adresse du handler (fonction à appeler)
- Le sélecteur de segment (CS à charger)
- Le DPL (qui peut déclencher cette interruption)

### Nos interruptions

| Vecteur | Source | DPL | Usage |
|---------|--------|-----|-------|
| 14 | CPU (Page Fault) | 0 | Debug seulement |
| 32 | PIC (IRQ0 Timer) | 0 | Préemption |
| 0x80 | Software (int 0x80) | 3 | Syscall |

### Pourquoi DPL=3 pour le syscall ?

Le DPL contrôle **qui peut déclencher** l'interruption avec `int N`.

- DPL=0 : Seul Ring 0 peut faire `int N`
- DPL=3 : Ring 3 peut aussi faire `int N`

Pour le syscall, on veut que les tâches user puissent l'appeler → DPL=3.

Pour le timer, c'est le hardware (PIC) qui déclenche, pas une instruction `int` → DPL=0 suffit.

### Le handler d'interruption

Quand une interruption arrive, le CPU :

1. Pousse SS, ESP (si changement de ring)
2. Pousse EFLAGS
3. Pousse CS, EIP
4. (Si exception avec code d'erreur) Pousse le code d'erreur
5. Charge CS:EIP depuis l'IDT
6. Exécute le handler

Pour revenir : `IRET` dépile tout dans l'ordre inverse.

### Notre syscall handler

```c
void syscall_isr(void) {
    asm volatile (
        "leave               \n"  // Défait le prologue du compilateur
        "pusha               \n"  // Sauve tous les registres
        "mov %%esp, %%eax    \n"  // Passe ESP comme argument
        "call syscall_handler\n"  // Appelle la vraie fonction
        "popa                \n"  // Restaure les registres
        "iret"                    // Retourne en Ring 3
        ::: "memory"
    );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx) {
    // ctx pointe vers la pile, qui contient le contexte d'interruption
    uint32_t* counter = (uint32_t*)ctx->gpr.esi.raw;  // Argument passé via ESI
    debug("[SYSCALL] Counter = %d\n", *counter);
}
```

Pourquoi `leave` ? Le compilateur génère un prologue (`push ebp; mov esp, ebp`) qu'on doit annuler car on veut contrôler exactement la pile.

Pourquoi `__regparm__(1)` ? Ça dit au compilateur de passer le premier argument dans EAX, pas sur la pile.

---

## 6. Le Scheduler

### C'est quoi ?

Le scheduler décide **quelle tâche tourne** à quel moment.

### Notre algorithme : Round-Robin

Super simple :
1. Timer interrompt
2. On passe à la tâche suivante
3. Répéter

```c
int next = (current + 1) % NB_TASKS;  // 0 → 1 → 0 → 1 → ...
```

### Le Context Switch (changement de contexte)

C'est la partie la plus délicate. Quand on change de tâche, on doit :

1. **Sauvegarder** l'état de la tâche courante
2. **Restaurer** l'état de la nouvelle tâche

L'état = tous les registres + où on en était dans le code (EIP).

### Comment on sauvegarde ?

Quand le timer interrompt, le CPU a déjà poussé sur la pile :
- SS, ESP (de Ring 3)
- EFLAGS
- CS, EIP

Puis notre handler fait `pusha` qui pousse tous les registres généraux.

ESP pointe maintenant vers une structure `int_ctx_t` complète !

```c
tasks[current].esp = (uint32_t)ctx;  // On sauvegarde ce pointeur
```

### Comment on restaure ?

Si la tâche a déjà été exécutée (esp != 0) :

```c
asm volatile (
    "mov %0, %%esp    \n"  // Charger le contexte sauvegardé
    "popa             \n"  // Restaurer les registres
    "iret"                 // Retourner (restaure CS, EIP, EFLAGS, SS, ESP)
    :: "r"(tasks[next].esp)
);
```

### Première exécution d'une tâche

La première fois, il n'y a rien de sauvegardé ! On doit **fabriquer** un faux contexte :

```c
if (tasks[next].esp == 0) {
    // Première exécution - on crée le contexte "de toutes pièces"
    asm volatile (
        "push %0      \n"  // SS (Ring 3)
        "push %1      \n"  // ESP (pile user)
        "push %2      \n"  // EFLAGS (avec IF=1 pour les interruptions)
        "push %3      \n"  // CS (Ring 3)
        "push %4      \n"  // EIP (adresse de user1 ou user2)
        "iret"
    );
}
```

L'`IRET` va dépiler tout ça et "croire" qu'on retourne d'une interruption vers Ring 3.

### Mise à jour du TSS

À chaque switch, on met à jour la pile kernel dans le TSS :

```c
tss->s0.esp = tasks[next].kstack + STACK_SIZE;
```

Pourquoi ? La prochaine interruption depuis cette tâche utilisera cette pile.

---

## 7. Le Flux Complet

### Démarrage

```
1. tp() est appelé par le kernel
   │
2. init_gdt() - Configure les segments
   │
3. init_tss() - Configure le TSS
   │
4. init_pagination() - Active la pagination
   │
5. setup_idt() - Configure les handlers
   │
6. init_tasks() - Initialise les structures de tâches
   │
7. start_scheduler() - Lance la première tâche
   │
   └──► IRET vers user1() en Ring 3
```

### Exécution normale

```
user1() tourne en Ring 3
          │
          │ ← Timer IRQ0 arrive !
          ▼
CPU: Sauvegarde SS,ESP,EFLAGS,CS,EIP sur pile kernel
          │
          ▼
timer_isr() en Ring 0
          │
          ├─ Acquitte le PIC (outb 0x20, 0x20)
          │
          ├─ Sauvegarde contexte Task 1
          │
          ├─ Passe à Task 2
          │
          ├─ Met à jour TSS
          │
          └─ IRET vers user2()
```

### Syscall

```
user2() veut afficher le compteur
          │
          ▼
sys_counter(&counter)
          │
          ├─ mov counter, %esi
          └─ int 0x80
                │
                ▼
syscall_isr() en Ring 0
                │
                ├─ Récupère ESI (l'adresse)
                ├─ Déréférence pour lire la valeur
                ├─ debug() pour afficher
                └─ IRET vers user2()
```

---

## 8. Les Pièges Courants

### Piège 1 : Les piles grandissent vers le bas

```c
// FAUX
tss.s0.esp = KERNEL_STACK_T1;  // Pointe vers le bas de la pile !

// CORRECT
tss.s0.esp = KERNEL_STACK_T1 + STACK_SIZE;  // Pointe vers le sommet
```

### Piège 2 : Les sélecteurs avec RPL

```c
// FAUX - RPL=0 pour un segment DPL=3
push 0x20   // Sélecteur data Ring 3, mais RPL=0

// CORRECT - RPL=3
push 0x23   // Sélecteur data Ring 3, RPL=3
```

Le CPU vérifie : `max(CPL, RPL) <= DPL`

### Piège 3 : EFLAGS et le bit IF

Si IF=0 (interruptions désactivées) dans les EFLAGS pushés pour IRET, la tâche tournera sans jamais être interrompue !

```c
uint32_t eflags = 0x200;  // Bit 9 = IF = 1
```

### Piège 4 : Oublier d'acquitter le PIC

Si on ne dit pas au PIC "OK j'ai traité l'IRQ", il ne générera plus d'interruption !

```c
outb(0x20, 0x20);  // EOI (End Of Interrupt) au PIC1
```

### Piège 5 : Les droits de pagination combinés

Les droits PDE et PTE sont combinés avec AND logique !

```
PDE avec PG_RW mais PTE sans PG_RW → Page read-only
PDE avec PG_USR mais PTE sans PG_USR → Page kernel only
```

Il faut mettre les droits aux deux niveaux :
```c
pg_set_entry(&pgd[i], PG_USR|PG_RW, ...);  // PDE
pg_set_entry(&ptb[j], PG_USR|PG_RW, ...);  // PTE
```

### Piège 6 : Démasquer l'IRQ dans le PIC

Par défaut, toutes les IRQ sont masquées. Il faut activer IRQ0 :

```c
uint8_t mask = inb(PIC1 + 1);  // Lire le masque actuel
outb(mask & 0xFE, PIC1 + 1);   // Bit 0 = 0 → IRQ0 démasquée
```

### Piège 7 : Le prologue du compilateur

Le compilateur génère `push ebp; mov esp, ebp` au début de chaque fonction. Si on veut contrôler la pile exactement, il faut l'annuler avec `leave`.

### Piège 8 : Calcul des index PGD

```c
// FAUX - 0x900000 n'est pas PGD[9] !
0x900000 >> 22 = 2  // C'est PGD[2] !

// Car chaque entrée PGD couvre 4MB = 0x400000
// 0x900000 / 0x400000 = 2.25 → PGD[2]
```

---

## Récapitulatif Final

```
┌─────────────────────────────────────────────────────────────────────┐
│                        ARCHITECTURE GLOBALE                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   GDT                    TSS                    IDT                 │
│   ┌───┐                 ┌───┐                  ┌───┐                │
│   │ 0 │ NULL            │ss0│──► Pile Kernel   │14 │ Page Fault     │
│   │ 1 │ Code R0         │esp│                  │32 │ Timer          │
│   │ 2 │ Data R0         └───┘                  │80 │ Syscall        │
│   │ 3 │ Code R3                                └───┘                │
│   │ 4 │ Data R3                                                     │
│   │ 5 │ TSS ─────────────────┘                                      │
│   └───┘                                                             │
│                                                                     │
│   PAGINATION                                                        │
│   ┌─────┐     ┌─────┐                                               │
│   │ PGD │────►│PTB 0│──► 0-4MB (identity)                          │
│   │     │────►│PTB 1│──► 4-8MB (identity)                          │
│   │     │────►│PTB 2│──► 8-12MB (identity)                         │
│   │     │────►│PTB 3│──► 0xC00000 → 0x500000 (shared T1)           │
│   │     │────►│PTB 4│──► 0x1000000 → 0x500000 (shared T2)          │
│   └─────┘     └─────┘                                               │
│                                                                     │
│   TÂCHES                                                            │
│   ┌──────────┐    ┌──────────┐                                      │
│   │  Task 1  │    │  Task 2  │                                      │
│   │  user1() │    │  user2() │                                      │
│   │          │    │          │                                      │
│   │ K-Stack  │    │ K-Stack  │                                      │
│   │ 0x600000 │    │ 0x601000 │                                      │
│   │          │    │          │                                      │
│   │ U-Stack  │    │ U-Stack  │                                      │
│   │ 0x700000 │    │ 0x701000 │                                      │
│   └──────────┘    └──────────┘                                      │
│         │              │                                            │
│         └──────┬───────┘                                            │
│                ▼                                                    │
│         ┌──────────┐                                                │
│         │ Shared   │  0x500000 (physique)                          │
│         │ Memory   │  = compteur partagé                           │
│         └──────────┘                                                │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Questions Fréquentes

**Q: Pourquoi on n'utilise pas un PGD différent par tâche ?**

R: On pourrait, mais c'est plus simple ainsi. Les deux tâches voient la même mémoire sauf pour la zone partagée qui est mappée différemment. Comme on utilise un PGD unique, les deux mappings de la zone partagée coexistent (PGD[3] et PGD[4]).

**Q: Comment Task 2 voit les modifications de Task 1 ?**

R: Les deux adresses virtuelles (0xC00000 et 0x1000000) pointent vers la même page physique (0x500000). Quand Task 1 écrit, ça modifie la RAM à 0x500000. Quand Task 2 lit, elle lit cette même RAM.

**Q: Pourquoi le kernel est identity-mapped ?**

R: C'est plus simple. Le kernel est chargé par GRUB à une adresse physique fixe (0x300000). Si on gardait la même adresse virtuelle, pas besoin de recalculer les pointeurs.

**Q: C'est quoi `__regparm__(1)` ?**

R: C'est un attribut GCC qui dit "passe le premier argument dans EAX au lieu de la pile". On l'utilise car juste avant le `call`, on a fait `mov %esp, %eax`.

**Q: Pourquoi `leave` avant `pusha` dans les handlers ?**

R: Le compilateur génère un prologue standard (`push ebp; mov esp, ebp`). On veut que `pusha` sauvegarde les vrais registres au moment de l'interruption, pas après que le prologue les ait modifiés. `leave` fait `mov ebp, esp; pop ebp` pour annuler le prologue.

