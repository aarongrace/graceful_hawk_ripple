/* File: cpu.c
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Mar. 6, 1996
   Revised: Feb. 27, 2002 - replace BCDGET with EX3ADJ instruction
   Revised: Mar.  1, 2002 - change condcodes after ADDSR and ADDSRU, add SSQADJ
   Revised: Mar. 13, 2002 - add BTRUNC
   Revised: July 18, 2002 - replace *ADJ with ADJUST, recode B* branches
   Revised: July 22, 2002 - MOVESL added.
   Revised: July 25, 2002 - recode Fxxx opcodes, add LOADL, STOREC
   Revised: Dec  31, 2007 - switch byte order of IR
   Revised: Aug  22, 2008 - fix to use stdint.h
   Revised: Oct  31, 2009 - fix MOVESL, ADDSR
   Revised: Aug  21, 2011 - add floating point coprocessor
   Revised: June 19, 2014 - add ADDJUST PLUSx and PLUS, fix startup bugs
   Revised: July 18, 2014 - add Sparrowhawk subset, fix bugs in virtualization
   Revised: Nov   7, 2019 - add include console.h, newstyle functions (finally)
   Revised: Dec  16, 2019 - make LOAD, LOADS, LIL allow dst=PC
   Revised: Nov   8, 2023 - change stdint.h to inttypes.h
   Revised: Dec  11, 2023 - make interrupts work

   Language: C (UNIX)
   Purpose: Hawk instruction set emulator
*/

/* First, declare that this is a main program */
#define MAIN

#include <inttypes.h>
#include "bus.h"
#include "powerup.h"
#include "console.h"
#include "float.h"


/************************************************************/
/* Declarations of machine components not included in bus.h */
/************************************************************/

static WORD irb; /* instruction register buffer */
static HALF ir;  /* the instruction register */

/* ir fields OP DST S1 S2 | OP DST OP1 SRC | OP DST OP1 X | OP DST CONST */
#include "irfields.h"

static WORD ea;     /* the effective address */

static WORD snoop;  /* the snooping address (LOADL,STOREC) */

/* the following are logically part of psw, but are computationally
   expensive, so not packed into PSW except when needed
*/
static WORD carries; /* the carry bits from the adder */
static WORD imask;   /* which interrupts are enabled (from LEVEL field) */


/**********************/
/* Arithmetic Support */
/**********************/

/* sign extend byte to word */
#define SXTB(x) {			\
	if (x & 0x00000080UL)		\
		x |= 0xFFFFFF00UL;	\
	else				\
		x &= 0x000000FFUL;	\
}

/* sign extend halfword to word */
#define SXTH(x) {			\
	if (x & 0x00008000UL)		\
		x |= 0xFFFF0000UL;	\
	else				\
		x &= 0x0000FFFFUL;	\
}

/* add (a macro so you can redefine it if overflow is trapped) */
#define ADDTO(x,y) x += y;

/* add with carry setting condition codes */
/* here, s is the sum discounting carries into each bit position,
   the sign bit of tells if the signs of the operands differed, the
   sign bit of o asks if signs of the operands are the same and the sign
   of result is different, and the sign of c gives carry out of sign
*/
#define ADDTOCC(x,yy,cin) {		\
	WORD y = yy;			\
	WORD s = (x ^ y);		\
	WORD c,v;			\
	x += (y + (cin));		\
	carries = s ^ x;		\
	v = ~s & (x ^ y);		\
	c = v ^ carries;		\
	psw &= ~(CC | CBITS);		\
	if (x & 0x80000000UL) psw |= N;	\
	if (x == 0x00000000UL) psw |= Z;\
	if (v & 0x80000000UL) psw |= V;	\
	if (c & 0x80000000UL) psw |= C;	\
}

/* pack up the psw */
/* this puts any fields of the PSW that should be part of the word into
   position so inspection of the word shows nice things.  The primary
   problem is to put carries into CBITS.
*/
#define PACKPSW {			\
	WORD srcbit = 0x00000010UL;	\
	WORD dstbit = 0x00000100UL;	\
	while (srcbit) {		\
		if (carries & srcbit) {	\
			psw |= dstbit;	\
		}			\
		srcbit <<= 4;		\
		dstbit <<= 1;		\
	}				\
	if (psw & C) psw |= dstbit;	\
}
#define UNPACKPSW {			\
	WORD dstbit = 0x00000010UL;	\
	WORD srcbit = 0x00000100UL;	\
	while (dstbit) {		\
		if (psw & srcbit) {	\
			carries |= dstbit;\
                }			\
                dstbit <<= 4;		\
		srcbit <<= 1;		\
        }				\
	imask = 0xFF >> (7 - PRIORITY); \
}

/* set condition codes for operations not involving the adder */
#define SETCC(x) {			\
	psw &= ~(CC | CBITS);		\
	carries = 0;			\
	if (x & 0x80000000UL) psw |= N;	\
	if (x == 0x00000000UL) psw |= Z;\
	/* V and C get reset */		\
}

/* set C condition code for load operations that detect null bytes */
#define SETNULLS(x) {			\
	if (!(x & 0x000000FFUL)) { psw |= C; } \
	if (!(x & 0x0000FF00UL)) { psw |= C; } \
	if (!(x & 0x00FF0000UL)) { psw |= C; } \
	if (!(x & 0xFF000000UL)) { psw |= C; } \
}

/* condition evaluation */
#define COND(x) (cctab[psw & CC] & (1 << x))
#define T   0x0001
#define NS  0x0002
#define ZS  0x0004
#define VS  0x0008
#define CS  0x0010
#define LT  0x0020
#define LE  0x0040
#define LEU 0x0080
/*      --- 0x0100 */
#define NR  0x0200
#define ZR  0x0400
#define VR  0x0800
#define CR  0x1000
#define GE  0x2000
#define GT  0x4000
#define GTU 0x8000

static unsigned int cctab[16] = {
	/* .... */ ( T | NR | ZR | VR | CR | GE | GT | LEU ),
	/* ...C */ ( T | NR | ZR | VR | CS | GE | GT | GTU ),
	/* ..V. */ ( T | NR | ZR | VS | CR | LT | LE | LEU ),
	/* ..VC */ ( T | NR | ZR | VS | CS | LT | LE | GTU ),
	/* .Z.. */ ( T | NR | ZS | VR | CR | GE | LE | LEU ),
	/* .Z.C */ ( T | NR | ZS | VR | CS | GE | LE | LEU ),
	/* .ZV. */ ( T | NR | ZS | VS | CR | LT | LE | LEU ),
	/* .ZVC */ ( T | NR | ZS | VS | CS | LT | LE | LEU ),
	/* N... */ ( T | NS | ZR | VR | CR | LT | LE | LEU ),
	/* N..C */ ( T | NS | ZR | VR | CS | LT | LE | GTU ),
	/* N.V. */ ( T | NS | ZR | VS | CR | GE | GT | LEU ),
	/* N.VC */ ( T | NS | ZR | VS | CS | GE | GT | GTU ),
	/* NZ.. */ ( T | NS | ZS | VR | CR | LT | LE | LEU ),
	/* NZ.C */ ( T | NS | ZS | VR | CS | LT | LE | LEU ),
	/* NZV. */ ( T | NS | ZS | VS | CR | GE | LE | LEU ),
	/* NZVC */ ( T | NS | ZS | VS | CS | GE | LE | LEU )
};

/********************/
/* Input Output Bus */
/********************/

static WORD input( addr )
WORD addr;
{
	if ((addr >= DISPBASE) && (addr <= DISPLIMIT)) {
		return dispread( addr );
	} else if ((addr >= KBDBASE) && (addr <= KBDLIMIT)) {
		return kbdread( addr );
	}
	return 0xAAAAAAAA;
}

static void output( WORD addr, WORD value ) {
	if ((addr >= DISPBASE) && (addr <= DISPLIMIT)) {
		dispwrite( addr, value );
	} else if ((addr >= KBDBASE) && (addr <= KBDLIMIT)) {
		kbdwrite( addr, value );
	}
}

/*******************************/
/* Instruction Execution Cycle */
/*******************************/

WORD lastpc;	/* the value of PC used to fetch the current instruction */

/* force a trap to vector - vector must be x_TRAP for some x */
#define TRAP( vector ) {				\
	tpc = lastpc;					\
	pc = vector;					\
	psw &= ~OLEVEL;					\
	psw |= (psw >> 4) & OLEVEL;			\
	psw &= ~LEVEL;					\
	UNPACKPSW;					\
}

/* after assign to PC, check for zero to allow zero as special breakpoint */
#define BRANCHCHECK {					\
	if (pc == 0) {					\
		morecycles = morecycles + cycles;	\
		cycles = 0;				\
	}						\
}

/* fetch one word relative to PC, basis of instruction fetch */
#define FETCHW {					\
	if (pc >= MAXMEM) { /* fetch is illegal */	\
		tma = pc;				\
		TRAP( BUS_TRAP );			\
	}						\
	/* fetch is legal */				\
	irb = m[(WORD)(pc >> 2)];			\
	cycles++;					\
}

/* fetch one halfword relative to PC, using FETCHW every other halfword */
#define FETCH(r) { /* r = m[pc](halfword); pc += 2 */	\
	if (pc & 0x2) { /* odd halfword */		\
		r = irb >> 16;				\
		pc += 2;				\
		FETCHW; /* fetch next instr */		\
	} else { /* even halfword */			\
		r = irb & 0xFFFF;			\
		pc += 2;				\
	}						\
}

#define LOAD(dst) { /* setup to load from memory */	\
	if (ea >= MAXMEM) { /* load outside memory */	\
		if (ea < IOSPACE) {			\
			tma = ea;			\
			TRAP( BUS_TRAP );		\
			FETCHW;				\
			continue;			\
		}					\
		dst = input( ea );			\
	} else { /* load is normal */			\
		dst = m[ea >> 2];			\
	}						\
	cycles++;					\
}

#define STORE(src) { /* setup to store to memory */	\
	if (ea == snoop) snoop |= 1;   \
	if (ea >= MAXMEM) { /* store outside memory */	\
		if (ea < IOSPACE) {			\
			tma = ea;			\
			TRAP( BUS_TRAP );		\
			FETCHW;				\
			continue;			\
		}					\
		output( ea, src );			\
	} else if (ea < MAXROM) { /* store is illegal */\
		tma = ea;				\
		TRAP( BUS_TRAP );			\
		FETCHW;					\
		continue;				\
	} else { /* store is normal */			\
		m[ea >> 2] = src;			\
	}						\
	cycles++;					\
}

int main(int argc, char ** argv) {
	breakpoint = 0; /* powerup may override this default */
	powerup(argc,argv);
	console_startup();

	cycles = 0;
	irq = 0;     /* no pending interrupts at startup */
	psw = 0;     /* all PSW fields zero at startup */
	imask = 0;   /* this is a consequence of PSW level field */
	carries = 0; /* this is a consequence of PSW carries field */
	FETCHW; /* fetch the first 2 instructions */
	for (;;) {
		if ( (!(cycles & 0x80000000UL))   /* positive -> display updt */
		||   (pc == breakpoint)         ) /* we reach breakpoint */
		/* then */ {
			PACKPSW;
			console();
		}

		lastpc = pc;

		{
			WORD intr = irq & imask;
			if (intr) { /* pending interrupt */
				WORD vector = INTERRUPT_TRAP;
				while ((intr & 1) == 0) { /* which interrupt */
					intr = intr >> 1;
					vector = vector + TRAP_VECTOR_STEP;
				}
				TRAP( vector );
				FETCHW;
				continue;
			}
		}

		FETCH(ir);

		r[0] = 0UL; /* force R0 to 0 before each instr */

		/***********************************
		 * in all of the following cases,  *
		 * normal exit is by continue,     *
		 * meaning fetch the next instr,   *
		 * while abnormal exit is by break *
		 * leading to an instructio trap   *
		 ***********************************/
		switch (OP) { /* decode the instruction */

		case 0xF: /* memory reference formats */

			switch (OP1) { /* decode secondary opcode field */

			case 0xF: /* MOVE */
				if (DST == 0) break;
				ea = 0;
				r[0] = pc;
				ADDTO(ea,r[X]);
				r[DST] = ea;
				continue;

			case 0xE: /* MOVECC */
				ea = 0;
				r[0] = pc;
				ADDTOCC(ea,r[X],0);
				r[DST] = ea;
				continue;

			case 0xD: /* LOADS */
				r[0] = pc;
				ea = r[X] & 0xFFFFFFFCUL;
				LOAD(r[DST]);
				if (DST != 0) continue;
				pc = r[0];
				BRANCHCHECK;
				FETCHW;
				continue;

			case 0xC: /* LOADSCC */
				r[0] = pc;
				ea = r[X] & 0xFFFFFFFCUL;
				LOAD(r[DST]);
				SETCC(r[DST]);
				SETNULLS(r[DST]);
				continue;

			case 0xB: /* JSRS */
				r[0] = pc;
				ea = r[X];
				r[DST] = pc;
				pc = ea;
				BRANCHCHECK;
				FETCHW;
				continue;

			case 0xA: /* STORES */
				if (X == 0) break;
				r[0] = pc;
				ea = r[X] & 0xFFFFFFFCUL;
				r[0] = 0;
				STORE(r[DST]);
				continue;

			case 0x9: /* -- LOADL, LOADSCC with added snooping */
				if (X == 0) break;
				r[0] = pc;
				ea = r[X] & 0xFFFFFFFCUL;
				snoop = ea;
				LOAD(r[DST]);
				SETCC(r[DST]);
				SETNULLS(r[DST]);
				continue;

			case 0x8: /* -- STOREC, STORES with snoop-driven fail */
				if (X == 0) break;
				r[0] = pc;
				ea = r[X] & 0xFFFFFFFCUL;
				r[0] = 0;
				psw &= ~(CC | CBITS);
				if (ea == snoop) {
					STORE(r[DST]);
				} else {
					psw |= V;
				}
				continue;

			case 0x7: /* LEA */
				#ifdef SPARROWHAWK
					break;
				#else
					if (DST == 0) break;
					FETCH(ea);
					SXTH(ea);
					r[0] = pc;
					ADDTO(ea,r[X]);
					r[DST] = ea;
					continue;
				#endif

			case 0x6: /* LEACC */
				#ifdef SPARROWHAWK
					break;
				#else
					FETCH(ea);
					SXTH(ea);
					r[0] = pc;
					ADDTOCC(ea,r[X],0);
					r[DST] = ea;
					continue;
				#endif

			case 0x5: /* LOAD */
				#ifdef SPARROWHAWK
					break;
				#else
					FETCH(ea);
					SXTH(ea);
					r[0] = pc;
					ADDTO(ea,r[X]);
					ea &= 0xFFFFFFFCUL;
					LOAD(r[DST]);
					if (DST != 0) continue;
					pc = r[0];
					BRANCHCHECK;
					FETCHW;
					continue;
				#endif

			case 0x4: /* LOADCC */
				#ifdef SPARROWHAWK
					break;
				#else
					FETCH(ea);
					SXTH(ea);
					r[0] = pc;
					ADDTO(ea,r[X]);
					ea &= 0xFFFFFFFCUL;
					LOAD(r[DST]);
					SETCC(r[DST]);
					SETNULLS(r[DST]);
					continue;
				#endif

			case 0x3: /* JSR */
				#ifdef SPARROWHAWK
					break;
				#else
					FETCH(ea);
					SXTH(ea);
					r[0] = pc;
					ADDTO(ea,r[X]);
					r[DST] = pc;
					pc = ea;
					BRANCHCHECK;
					FETCHW;
					continue;
				#endif

			case 0x2: /* STORE */
				#ifdef SPARROWHAWK
					break;
				#else
					FETCH(ea);
					SXTH(ea);
					r[0] = pc;
					ADDTO(ea,r[X]);
					ea &= 0xFFFFFFFCUL;
					r[0] = 0;
					STORE(r[DST]);
					continue;
				#endif

			case 0x1: /* -- */
			case 0x0: /* -- */
				break;

			}
			/* only traps get here */
			break;

		case 0xE: /* LIL */
			#ifdef SPARROWHAWK
				break;
			#else
				r[DST] = CONST;
				FETCH(ea);
				SXTH(ea);
				r[DST] |= (ea << 8);
				if (DST != 0) continue;
				pc = r[0];
				BRANCHCHECK;
				FETCHW;
				continue;
			#endif

		case 0xD: /* LIS */
			if (DST == 0) break;
			r[DST] = CONST;
			SXTB(r[DST]);
			continue;

		case 0xC: /* ORIS */
			if (DST == 0) break;
			r[DST] <<= 8;
			r[DST] |= CONST;
			continue;

		case 0xB: /* MOVESL */
			if (S1 == 0) break;
			{
				int shift = (S2 - 1) & 0xF;
				WORD d = r[S1];
				WORD c = d & ~(0x7FFFFFFFUL >> shift);
				WORD vm = 0x7FFFFFFFUL >> (shift + 1);
				WORD v = d & ~vm;
				r[DST] = d << (shift + 1);
				SETCC(r[DST]);
				if (c) psw |= C;
				if (v) v = (v + vm + 1) & vm;
				if (v) psw |= V;
			}
			continue;

		case 0xA: /* ADDSL */
			if (DST == 0) break;
			{
				int shift = (S2 - 1) & 0xF;
				WORD d = r[DST];
				WORD c = d & ~(0x7FFFFFFFUL >> shift);
				WORD vm = 0x7FFFFFFFUL >> (shift + 1);
				WORD v = d & ~vm;
				d <<= (shift + 1);
				ADDTOCC(d,r[S1],0);
				r[DST] = d;
				if (c) psw |= C;
				if (v) v = (v + vm + 1) & vm;
				if (v) psw |= V;
			}
			continue;

		case 0x9: /* ADDSR */
			{
				int shift = ((S2 - 1) & 0xF) + 1;
				WORD d = r[DST];
				WORD v;
				WORD c;
				WORD m = 0x7FFFFFFFUL >> (shift - 1);
				ADDTOCC(d,r[S1],0);
				v = d & ~(0xFFFFFFFFUL << shift);
				c = d &  (0x00000001UL << (shift - 1));
				d >>= shift;
				if (psw & N) {
					if (psw & V) {  /* neg and ovf */
						d &= m; /*   make positive */
					} else {        /* neg and no ovf */
						d |= ~m;/*   make negative */
					}
				} else {
					if (psw & V) {  /* pos and ovf */
						d |= ~m;/*   make negative */
					} else {        /* pos and no ovf */
						d &= m; /*   make positive */
					}
				}
				SETCC(d);
				r[DST] = d;
				if (v) psw |= V;
				if (c) psw |= C;
			}
			continue;

		case 0x8: /* ADDSRU */
			{
				int shift = ((S2 - 1) & 0xF) + 1;
				WORD d = r[DST];
				WORD v;
				WORD c;
				WORD m = 0x7FFFFFFFUL >> (shift - 1);
				ADDTOCC(d,r[S1],0);
				v = d & ~(0xFFFFFFFFUL << shift);
				c = d &  (0x00000001UL << (shift - 1));
				d >>= shift;
				d &= m;
				if (psw & C) {
					d += (m + 1);
				}
				SETCC(d);
				r[DST] = d;
				if (v) psw |= V;
				if (c) psw |= C;
			}
			continue;

		case 0x7: /* STUFFB */
			if (DST == 0) break;
			{
				int d = DST;
				int shift = ((int)(r[S2] & 3)) << 3;
				WORD mask = ~(0x000000FFUL << shift);
				r[d] = (r[d] & mask)
				     | ((r[S1] & 0x000000FFUL) << shift);
			}
			continue;

		case 0x6: /* STUFFH */
			if (DST == 0) break;
			{
				int d = DST;
				int shift = ((int)(r[S2] & 2)) << 3;
				WORD mask = ~(0x0000FFFFUL << shift);
				r[d] = (r[d] & mask)
				     | ((r[S1] & 0x0000FFFFUL) << shift);
			}
			continue;

		case 0x5: /* EXTB */
			if (S1 == 0) break;
			{
				int shift = ((int)(r[S2] & 3)) << 3;
				WORD src = r[S1];
				r[DST] = (src >> shift) & 0x000000FFUL;
				SETCC(r[DST]);
			}
			continue;

		case 0x4: /* EXTH */
			if (S1 == 0) break;
			{
				int shift = ((int)(r[S2] & 2)) << 3;
				WORD src = r[S1];
				r[DST] = (src >> shift) & 0x0000FFFFUL;
				SETCC(r[DST]);
			}
			continue;

		case 0x3: /* ADD */
			if (S1 == 0) break;
			if (S2 == 0) break;
			{
				WORD dst = r[S1];
				ADDTOCC(dst,r[S2],0);
				r[DST] = dst;
			}
			continue;

		case 0x2: /* SUB */
			if (S2 == 0) break;
			{
				WORD dst = r[S1];
				ADDTOCC(dst,~r[S2],1);
				r[DST] = dst;
			}
			continue;

		case 0x1: /* Two register format */
				
			switch (OP1) { /* decode secondary opcode field */

			case 0xF: /* TRUNC */
				if (DST == 0) break;
				{
					int s = (SRC - 1) & 0xF;
					WORD m = 0xFFFFFFFFUL << s;
					WORD d = r[DST];
					WORD g = d & m;
					WORD c = d & (m<<1);
					d &= ~(m<<1);
					SETCC(d);
					r[DST] = d;
					if (c) psw |= C;
					if (g) g = (~g) & m;
					if (g) psw |= V;
				}
				continue;

			case 0xE: /* SXT */
				if (DST == 0) break;
				{
					int s = (SRC - 1) & 0xF;
					WORD m = 0xFFFFFFFFUL << s;
					WORD d = r[DST];
					WORD g = d & m;
					WORD c = d & (m<<1);
					if (d&((~m)+1)) { /* negative */
						d |= m;
					} else { /* positive */
						d &= ~m;
					}
					SETCC(d);
					r[DST] = d;
					if (c) psw |= C;
					if (g) g = (~g) & m;
					if (g) psw |= V;
				}
				continue;

			case 0xD: /* BTRUNC */
				if (DST == 0) break;
				{
					int s = ((SRC - 1) & 0xF) + 1;
					WORD ms = 0xFFFFFFFFUL << s;
					WORD d = r[DST] & ~ms;
					ADDTO(pc, d << 1);
					BRANCHCHECK;
					FETCHW;
				}
				continue;

			case 0xC: /* ADDSI */
				if (DST == 0) break;
				{
					WORD src = SRC;
					if (src & 0x8) src |= 0xFFFFFFF0UL;
					if (src == 0) src = 8;
					ADDTOCC(r[DST], src, 0);
				}
				continue;

			case 0xB: /* AND */
				if (DST == 0) break;
				if (SRC == 0) break;
				r[DST] &= r[SRC];
				SETCC(r[DST]);
				continue;

			case 0xA: /* OR  */
				if (DST == 0) break;
				if (SRC == 0) break;
				r[DST] |= r[SRC];
				SETCC(r[DST]);
				continue;

			case 0x9: /* EQU */
				if (DST == 0) break;
				r[DST] = ~(r[DST] ^ r[SRC]);
				SETCC(r[DST]);
				continue;

			case 0x8: /* --  */
				break;

			case 0x7: /* ADDC */
				{
					int nz = (~psw) & Z;
					ADDTOCC(r[DST], r[SRC], psw & C);
					if (nz) psw &= ~Z;
				}
				continue;

			case 0x6: /* SUBB */
				{
					int nz = (~psw) & Z;
					ADDTOCC(r[DST], ~r[SRC], psw & C);
					if (nz) psw &= ~Z;
				}
				continue;

			case 0x5: /* ADJUST */
				if (DST == 0) break;
				{
					WORD src = 0; /* effective source */
					switch (SRC) { /* decode source */

					case 0x0: /*  */
					case 0x1: /*  */
						break;

					case 0x2: /* BCD */
						src = (carries>>1)
							& 0x08888888UL;
						if (psw & C)
							src |= 0x80000000UL;
						src = (src >> 1)|(src >> 2);
						src = (~src) + 1;
						/* subtract 6 from digits
						   that didn't produce carry */
						break;

					case 0x3: /* EX3 */
						src = (carries>>1)
							& 0x08888888UL;
						if (psw & C)
							src |= 0x80000000UL;
						src |=   src >> 2      ;
						src |= ((src << 1) | 1);
						src ^= 0xCCCCCCCCUL;
						/* either add or subtract 3
						   to make correct excess-3 */
						break;

					case 0x4: /* CMSB */
						if (psw & C)
							src = 0x80000000UL;
						break;

					case 0x5: /* SSQ */
						if ((psw & N) && (psw & V))
							src = 0x00000001UL;
						break;

					case 0x6: /*  */
					case 0x7: /*  */
						break;

					case 0x8: /* PLUS1 */
						src = 1;
						break;

					case 0x9: /* PLUS2 */
						src = 2;
						break;

					case 0xA: /* PLUS4 */
						src = 4;
						break;

					case 0xB: /* PLUS8 */
						src = 8;
						break;

					case 0xC: /* PLUS16 */
						src = 16;
						break;

					case 0xD: /* PLUS32 */
						src = 32;
						break;

					case 0xE: /* PLUS64 */
						src = 64;
						break;

					case 0xF: /* PLUS128 */
						src = 128;
						break;
					}
					if (DST != 0) {
						ADDTO(r[DST],src);
					}
				}
				continue;

			case 0x4: /* PLUS */
				if (DST == 0) break;
				r[0] = pc;
				WORD dst = r[S1];
				ADDTO(r[DST],r[S2]);
				continue;

			case 0x3: /* COGET */
			    #ifdef SPARROWHAWK
				break;
			    #else
				psw &= ~(CC | CBITS); /* always reset cc */
				if (SRC == 0) {
					if (costat == 0) psw |= Z;
					r[DST] = costat;
					continue;
				} else switch (COSEL) {
				case 0x0:
					break; /* missing coprocessor */
				case 0x1:
					if (!(costat & COFPENAB)) {
						TRAP( CO_TRAP );
						FETCHW;
						continue;
					}
					r[DST] = float_coget( SRC );
					psw |= cocc; /* cond codes from cop */
					continue;
				case 0x2:
				case 0x3:
				case 0x4:
				case 0x5:
				case 0x6:
				case 0x7:
					break; /* missing coprocessor */
				}
				TRAP( CO_TRAP );
				FETCHW;
				continue;
			    #endif

			case 0x2: /* COSET */
			    #ifdef SPARROWHAWK
				break;
			    #else
				if (SRC == 0) {
					costat = r[DST] & COMASK;
					continue;
				} else switch (COSEL) {
				case 0x0:
					break; /* missing coprocessor */
				case 0x1:
					if (!(costat & COFPENAB)) {
						TRAP( CO_TRAP );
						FETCHW;
						continue;
					}
					float_coset( SRC, r[DST] );
					continue;
				case 0x2:
				case 0x3:
				case 0x4:
				case 0x5:
				case 0x6:
				case 0x7:
					break; /* missing coprocessor */
				}
				TRAP( CO_TRAP );
				FETCHW;
				continue;
			    #endif

			case 0x1: /* CPUGET */
				if ((psw & LEVEL) == LEVEL) { /* all ones */
					TRAP( PRIV_TRAP );
					FETCHW;
					continue;
				}
				{
					WORD dst;
					switch (SRC) { /* decode source */

					case 0x0: /* PSW */
						PACKPSW;
						dst = psw;
						break;

					case 0x1: /* TPC */
						dst = tpc;
						break;

					case 0x2: /* TMA */
						dst = tma;
						break;

					case 0x3: /* TSV */
						dst = tsv;
						break;

					case 0x4: /* -- */
					case 0x5: /* -- */
					case 0x6: /* -- */
					case 0x7: /* -- */
						break;

					case 0x8: /* CYC */
						dst = cycles + morecycles;
						break;

					case 0x9: /* -- */
					case 0xA: /* -- */
					case 0xB: /* -- */
					case 0xC: /* -- */
					case 0xD: /* -- */
					case 0xE: /* -- */
					case 0xF: /* -- */
						break;
					}
					if (DST != 0) {
						r[DST] = dst;
					} else {
						pc = dst;
						psw &= ~LEVEL;
						psw |= ((psw & OLEVEL) << 4);
						psw &= ~OLEVEL;
						BRANCHCHECK;
						FETCHW;
					}
				}
				continue;
				/* control never reaches here */

			case 0x0: /* CPUSET */
				if ((psw & LEVEL) == LEVEL) { /* all ones */
					TRAP( PRIV_TRAP );
					FETCHW;
					continue;
				}
				switch (SRC) { /* decode terenary opcode */

				case 0x0: /* PSWSET */
					psw = r[DST];
					UNPACKPSW;
					continue;

				case 0x1: /* TPCSET */
					tpc = r[DST];
					continue;

				case 0x2: /* TMASET */
					tma = r[DST];
					continue;

				case 0x3: /* TSVSET */
					tsv = r[DST];
					continue;

				case 0x4: /* -- */
				case 0x5: /* -- */
				case 0x6: /* -- */
				case 0x7: /* -- */
					continue;

				case 0x8: /* CYCSET */
					morecycles = r[DST];
					cycles = 0;
					continue;

				case 0x9: /* -- */
				case 0xA: /* -- */
				case 0xB: /* -- */
				case 0xC: /* -- */
				case 0xD: /* -- */
				case 0xE: /* -- */
				case 0xF: /* -- */
					continue;

				}
				/* control never reaches here */
			}
			/* only traps get here */
			break;

		case 0x0: /* Bcc */
			/* CONST == 0xFF could be illegal, infinite loop */
			/* CONST == 0xFF could be sleep command */
			if (DST == 0x8) break;
			if (COND(DST)) {
				ea = CONST;
				SXTB(ea);
				ADDTO(pc, ea << 1);
				BRANCHCHECK;
				FETCHW;
			}
			continue;
		}
		/* only traps get here */
		tma = 0;
		TRAP( INSTRUCTION_TRAP );
		FETCHW;
	}
}
