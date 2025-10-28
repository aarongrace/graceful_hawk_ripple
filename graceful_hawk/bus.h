/* File: bus.h
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date:    Aug 1996
   Revised: Aug 22, 2008 -- fix to use stdint.h definitions
   Revised: Aug 21, 2011 -- add interface to floating point coprocessor
   Revised: Nov  8, 2023 -- minor changes to coprocessor masks, (WORD)casting
   Revised: Dec 11, 2023 -- interrupt support
   Language: C (UNIX)
   Purpose:
	Declarations of bus lines shared by the hawk CPU and peripherals.
	This is not, strictly speaking, a real bus layout,
	but rather, it is a set of declarations driven by the
	needs of system emulation.
   Constraints:
	When included in the main program, MAIN must be defined.
	When included elsewhere, MAIN must not be defined.
	In all cases <stdint.h> must be included for uintX_t types.
*/


/* The following trick puts extern on definitions if not in the main program
*/
#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif


/*********************/
/* Hawk data formats */
/*********************/

#define WORD uint32_t
#define SWORD int32_t
#define HALF uint16_t
#define BYTE uint8_t


/******************************************/
/* Utility information needed by emulator */
/******************************************/

/* maximum length of a sensible file name
 */
#define NAME_LENGTH 120

/* count of memory cycles; cycles is incremented with every memory
   reference and causes console interrupts.  the sum cycles+morecycles
   is the true cycle count.  recycle determines frequency of console update
 */
EXTERN WORD cycles;
EXTERN WORD morecycles;
EXTERN WORD recycle;

/* memory address compared with pc to stop cpu at breakpoints
 */
EXTERN WORD breakpoint;

extern int animation_mode;

/*****************************************************/
/* Globals that really aren't really part of the bus */
/*****************************************************/

EXTERN char * progname; /* name of program itself (argv[0]) */


/**********/
/* Memory */
/**********/

/* This emulator does not allow for non-contiguous memory fields.
   Memory runs from 0 to MAXMEM-1; MAXMEM should be a multiple of 0x10000,
   and should never be as great as 0xFFFF0000 (a highly unlikely limitation).
   Memory from location 0 to MAXROM-1 is read-only and may not be modified
   at run-time; MAXROM should be a multiple of 0x10000.
*/

/* MAXMEM is specified in bytes and provided by Makefile */
/* MAXROM is specified in bytes and provided by Makefile */

/* note that memory is word addressable */
EXTERN WORD m[ MAXMEM >> 2 ];

/* memory mapped I/O owns the top 16 meg of the address space */
#define IOSPACE   0xFF000000UL

/* the memory mapped display device -- give it a megabyte */
#define DISPBASE  0xFF000000UL
#define DISPLIMIT 0xFF0FFFFFUL

/* the minimalist keyboard */
#define KBDBASE   0xFF100000UL
#define KBDLIMIT  0xFF10000FUL

/* allow for an IBM PC style I/O address space is addressed in the last 256Kb
   so that least sig 2 bits of address are ignored, and next 16 bits are
   the 16 bit address of a PC-style in or out command */

/**************************/
/* Memory Management Unit */
/**************************/

/* TLB size and mask to select bits of TLB index */
#define TLBMASK 0xF
#define TLBSIZE (TLBMASK+1)

/* Fields of virtual and physical addresses */
#define PAGEBITS  12
#define PAGEFIELD (((WORD)0xFFFFFFFFUL << PAGEBITS) & (WORD)0xFFFFFFFFUL)
#define WORDFIELD ((~PAGEFIELD) & (WORD)0xFFFFFFFFUL)

/* Access rights bits */
#define ARGLOBAL 0x20
#define ARCACHE  0x10
#define ARREAD   0x08
#define ARWRITE  0x04
#define AREXEC   0x02
#define ARVALID  0x01

/********************************/
/* Trap and Interrupt Vectoring */
/********************************/

/* All trap addresses must be less than MAXMEM!
*/

#define RESTART_TRAP      (WORD)0x00000000UL  /* on powerup */
#define BUS_TRAP          (WORD)0x00000010UL  /* illegal physical address */
#define INSTRUCTION_TRAP  (WORD)0x00000020UL  /* illegal opcode */
#define PRIV_TRAP         (WORD)0x00000030UL  /* privilege violation */
#define MMU_TRAP          (WORD)0x00000040UL  /* mmu fault */
#define CO_TRAP           (WORD)0x00000050UL  /* missing coprocessor */

#define INTERRUPT_TRAP    (WORD)0x00000080UL  /* interrupts use top half */

#define TRAP_VECTOR_STEP  (WORD)0x00000010UL  /* spacing of vector entries*/

/*******************************/
/* Generally visible registers */
/*******************************/

/* All of the following are visible outside the CPU in some context or
   another, either to some I/O device or to the front panel.
*/
EXTERN WORD r[16];  /* the general purpose registers */
EXTERN WORD pc;     /* the program counter */
EXTERN WORD costat; /* the coprocessor status register */
EXTERN WORD cocc;   /* condition codes set by coprocessor for COGET */
EXTERN WORD psw;    /* the processor status word */
EXTERN WORD tpc;    /* saved pc after a trap */
EXTERN WORD tma;    /* saved memory address after a trap */
EXTERN WORD tsv;    /* trap save location */
EXTERN WORD irq;    /* interrupt request */

/* costat fields
*/
#define COOP   ((costat & (WORD)0x0F000) >> 12)
#define COSEL  ((costat & (WORD)0x00700) >> 8)
#define COENAB (WORD)0x000FE
/* each coprocessor defines its own coprocessor enable bit */
#define COFPENAB (WORD)0x00002
#define COMASK (0xF700 | COENAB)

/* psw fields
*/
#define N  (WORD)0x00000008UL
#define Z  (WORD)0x00000004UL
#define V  (WORD)0x00000002UL
#define C  (WORD)0x00000001UL
#define CC (N|V|Z|C)
#define CBITS (WORD)0x0000FF00UL
#define LEVEL (WORD)0xF0000000UL
#define OLEVEL (WORD)0x0F000000UL

/* priority interrupt scheme
   there are several ways to make the level field establish interrupt priority.
   16 levels are possible but the top bit enables the MMU.
   If the bottom 3 bits establish interrupt priority, we could
   -- each bit enables one interrupt, with software priorities 111 011 001 000.
   -- the three bits are a numeric level 111 down to 000.  We do that here.
   note:    If only levels 7, 3 and 1 are used, these are compatible.
*/
#define PRIORITY ((psw >> 28) & 7)

/* irq fields
   warning: if defined, IRQ0 is nonmaskable and will hang CPU, so don't!
   rule:    IRQi is higher priority IRQj if i < j
   note:    if only IRQ1, IRQ2 and IRQ4 are used, hardware can be simplified
*/
#define IRQ1  (WORD)0x00000002UL
#define IRQ2  (WORD)0x00000004UL
#define IRQ3  (WORD)0x00000008UL
#define IRQ4  (WORD)0x00000010UL
#define IRQ5  (WORD)0x00000020UL
#define IRQ6  (WORD)0x00000040UL
#define IRQ7  (WORD)0x00000080UL

