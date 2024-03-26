/* File: showop.c
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Apr. 23, 2002
   Revised: July 25, 2002 - matches revisions to cpu.c
   Revised: Dec  31, 2007 - matches revisions to cpu.c, improve display style
   Revised: Aug  22, 2008 - use stdint.h, (WORD)casting

   Language: C (UNIX) with -lcurses option
   Purpose: Hawk Emulator, disassembler for HAWK opcodes
*/

#include <inttypes.h>
#include <curses.h>
#include "bus.h"
#include "showop.h"

/**************************** 
 * HAWK instruction formats * 
 ****************************/

#define ILLEGAL  0
#define LONGMEM  1
#define SHORTMEM 2
#define LONGIMM  3
#define SHORTIMM 4
#define BRANCH   5
#define SHIFT    6
#define THREEREG 7
#define SHORTCON 8
#define TWOREG   9
#define SPECIAL  10
#define NOREG    11

/* ir fields OP DST S1 S2 | OP DST OP1 SRC | OP DST OP1 X | OP DST CONST */
#include "irfields.h"

/*****************************
 * internal support routines * 
 *****************************/

static char * name;     /* the textual name of the instruction */
static int form;        /* the instruction format */
static HALF ir;         /* the memory location */

static void decode( WORD a ) {
	/* decode the opcode in m[a] and stage results in static variables */
	name = NULL;     /* instruction has no name by default */
	form = ILLEGAL;  /* instruction is illegal format by default */

	if (a >= MAXMEM) return; /* above maxmem, all are illegal */

	/* fetch the instruction */
	if (a & 2) {
		ir = m[a>>2] >> 16;
	} else {
		ir = m[a>>2];
	}

	/* decode the instruciton, for its name and format */
	switch (OP) {

	case 0xF: /* memory reference formats */

		switch (OP1) { /* decode secondary opcode field */

		case 0xF: name = "MOVE    "; form = SHORTMEM; break;
		case 0xE: name = "MOVECC  "; form = SHORTMEM; break;
		case 0xD: name = "LOADS   "; form = SHORTMEM; break;
		case 0xC: name = "LOADSCC "; form = SHORTMEM; break;
		case 0xB: name = "JSRS    "; form = SHORTMEM; break;
		case 0xA: name = "STORES  "; form = SHORTMEM; break;
		case 0x9: name = "LOADL   "; form = SHORTMEM; break;
		case 0x8: name = "STOREC  "; form = SHORTMEM; break;
		case 0x7: name = "LEA     "; form = LONGMEM; break;
		case 0x6: name = "LEACC   "; form = LONGMEM; break;
		case 0x5: name = "LOAD    "; form = LONGMEM; break;
		case 0x4: name = "LOADCC  "; form = LONGMEM; break;
		case 0x3: name = "JSR     "; form = LONGMEM; break;
		case 0x2: name = "STORE   "; form = LONGMEM; break;
		case 0x1: break;
		case 0x0: break;

		}
		break;

	case 0xE: name = "LIL     "; form = LONGIMM; break;
	case 0xD: name = "LIS     "; form = SHORTIMM; break;
	case 0xC: name = "ORIS    "; form = SHORTIMM; break;
	case 0xB: name = "MOVESL  "; form = SHIFT; break;
	case 0xA: name = "ADDSL   "; form = SHIFT; break;
	case 0x9: name = "ADDSR   "; form = SHIFT; break;
	case 0x8: name = "ADDSRU  "; form = SHIFT; break;
	case 0x7: name = "STUFFB  "; form = THREEREG; break;
	case 0x6: name = "STUFFH  "; form = THREEREG; break;
	case 0x5: name = "EXTB    "; form = THREEREG; break;
	case 0x4: name = "EXTH    "; form = THREEREG; break;
	case 0x3: name = "ADD     "; form = THREEREG; break;
	case 0x2: name = "SUB     "; form = THREEREG; break;

	case 0x1: /* Two register format */
				
		switch (OP1) { /* decode secondary opcode field */

		case 0xF: name = "TRUNC   "; form = SHORTCON; break;
		case 0xE: name = "SXT     "; form = SHORTCON; break;
		case 0xD: name = "BTRUNC  "; form = SHORTCON; break;
		case 0xC: name = "ADDSI   "; form = SHORTCON; break;
		case 0xB: name = "AND     "; form = TWOREG; break;
		case 0xA: name = "OR      "; form = TWOREG; break;
		case 0x9: name = "EQU     "; form = TWOREG; break;
		case 0x8: break;
		case 0x7: name = "ADDC    "; form = TWOREG; break;
		case 0x6: name = "SUBB    "; form = TWOREG; break;
		case 0x5: name = "ADJUST  "; form = SPECIAL; break;
		case 0x4: name = "PLUS    "; form = TWOREG; break;
		case 0x3: name = "COGET   "; form = SPECIAL; break;
		case 0x2: name = "COSET   "; form = SPECIAL; break;
		case 0x1: name = "CPUGET  "; form = SPECIAL; break;
		case 0x0: name = "CPUSET  "; form = SPECIAL; break;
		}
		break;

	case 0x0: /* Bcc */

		switch (DST) { /* decode condition field */

		case 0xF: name = "BGTU    "; form = BRANCH; break;
		case 0xE: name = "BGT     "; form = BRANCH; break;
		case 0xD: name = "BGE     "; form = BRANCH; break;
		case 0xC: name = "BCR     "; form = BRANCH; break;
		case 0xB: name = "BVR     "; form = BRANCH; break;
		case 0xA: name = "BNE     "; form = BRANCH; break;
		case 0x9: name = "BNR     "; form = BRANCH; break;
		case 0x8: break;
		case 0x7: name = "BLEU    "; form = BRANCH; break;
		case 0x6: name = "BLE     "; form = BRANCH; break;
		case 0x5: name = "BLT     "; form = BRANCH; break;
		case 0x4: name = "BCS     "; form = BRANCH; break;
		case 0x3: name = "BVS     "; form = BRANCH; break;
		case 0x2: name = "BEQ     "; form = BRANCH; break;
		case 0x1: name = "BNS     "; form = BRANCH; break;
		case 0x0:
			if (ir == 0x0000) {
				name = "NOP     "; form = NOREG; break;
			}
			name = "BR      "; form = BRANCH; break;

		}
		break;
	}
}

static void showit(WORD a) {
	HALF next = 0; /* next word of instruction, if needed */

	/* fetch the next locaton, if needed */
	if ( ((a + 2) < MAXMEM)
        &&   ((form == LONGMEM) || (form == LONGIMM)) ) {
		if (a & 2) { /* ir was in the odd half */
			next = m[(a + 2) >> 2] & (WORD)0xFFFFUL;
		} else { /* ir was in the even half */
			next = (m[a >> 2] >> 16) & (WORD)0xFFFFUL;
		}
	}

	/* display it, depending on the format */
	if (form != ILLEGAL) {
		addstr(name);
		switch (form) {

		case LONGMEM:
			if (X != 0) { /* indexed */
				printw("R%1X,R%1X,#%04X",
					DST, X, next);
			} else { /* pc relative */
				WORD dst = next;
				if (next & 0x8000) dst |= (WORD)0xFFFF0000UL;
				dst += a + 4; 
				printw("R%1X,#%06"PRIX32, DST, dst);
			}
			break;
		case SHORTMEM:
			printw("R%1X,R%1X", DST, X);
			break;
		case LONGIMM:
			printw("R%1X,#%06X", DST, (next << 8) | CONST);
			break;
		case SHORTIMM:
			printw("R%1X,#%02X", DST, CONST);
			break;
		case BRANCH:
			{
				WORD dst = CONST;
				if (CONST & 128) dst |= (WORD)0xFFFFFF00UL;
				dst = (dst << 1) + (a + 2);
				printw("#%06"PRIX32, dst);
			}
			break;
		case SHIFT:
			printw("R%1X,R%1X,#%1X", DST, S1, S2);
			break;
		case THREEREG:
			printw("R%1X,R%1X,R%1X", DST, S1, S2);
			break;
		case SHORTCON:
			printw("R%1X,#%1X", DST, SRC);
			break;
		case TWOREG:
			printw("R%1X,R%1X", DST, SRC);
			break;
		case SPECIAL:
			printw("R%1X,#%1X", DST, SRC);
			break;
		case NOREG:
			break;
		}
	} else { /* illegal */
		printw("#%04"PRIX32, ir & (WORD)0x0000FFFFUL);
	}
}

static int mysize() {
	/* return instruction size */
	if ((form == LONGMEM) || (form == LONGIMM)) {
		return 4;
	} else {
		return 2;
	}
}

/*******************************
 * disassemble one instruction * 
 *******************************/

int showop( WORD a ) {
	/* decode the opcode in m[a] and output it; returns address increment */
	decode( a );
	showit( a );
	return mysize();
}

int sizeofop( WORD a ) {
	/* decode the opcode in m[a] and return address increment */
	decode( a );
	return mysize();
}
