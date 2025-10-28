/* File: float.c
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Aug. 21, 2011
   Revised: Nov.  8, 2023 -- add float_acc() for console display of state

   Language: C (UNIX)
   Purpose: Hawk floating point coprocessor
*/

/* First, declare that this is a main program */

#include <inttypes.h>
#include <math.h>
#include "bus.h"
#include "float.h"


/************************************************************/
/* Declarations of coprocessor state not included in bus.h  */
/************************************************************/

double fpa[2];	/* two floating accumulators, always in long format */
WORD fplow;

/* bit in COSTAT */
#define FPLONG 0x01000

/*************/
/* Interface */
/*************/

double float_acc( int i ) {
	/* read-only access to floating point accumulators for front panel */
	return fpa[i];
}

void float_coset( int reg, WORD val ) {
	/* coprocesor operation initiated by CPU */
	int a = reg & 1;
	int r = reg >> 1;
	if (!(costat & COFPENAB)) {
		return; /* floating point unit disabled */
	} else if (costat & FPLONG) { /* long mode */
		int64_t bigval = (((int64_t)val) << 32) | fplow;
		switch (r) {
		case 0: /* FPLOW -- context guarantes not COSTAT */
			fplow = val;
			break;
		case 1: /* FPA[r] */
			fpa[a] = *((double *)(&bigval));
			break;
		case 2: /* FPINT to fpa[a] */
			fpa[a] = bigval; /* bigval is already signed */
			break;
		case 3: /* FPSQRT to fpa[a] */
			fpa[a] = sqrt( *((double *)(&bigval)) );
			break;
		case 4: /* FPADD to fpa[a] */
			fpa[a] = fpa[a] + *((double *)(&bigval));
			break;
		case 5: /* FPSUB from fpa[a] */
			fpa[a] = fpa[a] - *((double *)(&bigval));
			break;
		case 6: /* FPMUL to fpa[a] */
			fpa[a] = fpa[a] * *((double *)(&bigval));
			break;
		case 7: /* FPDIV into fpa[a] */
			fpa[a] = fpa[a] / *((double *)(&bigval));
			break;
		}
	} else {
		switch (r) { /* short mode */
		/* all operands get lengthened, fplow is there but ignored */
		case 0: /* FPLOW -- context guarantes not COSTAT */
			fplow = val;
			break;
		case 1: /* FPA[a] */
			fpa[a] = *((float *)(&val)); /* lengthen operand */
			break;
		case 2: /* FPINT to fpa[a] */
			fpa[a] = (SWORD) val; /* converted value is signed */
			break;
		case 3: /* FPSQRT to fpa[a] */
			fpa[a] = sqrt( *((float *)(&val)) );
			break;
		case 4: /* FPADD to fpa[a] */
			fpa[a] = fpa[a] + *((float *)(&val));
			break;
		case 5: /* FPSUB from fpa[a] */
			fpa[a] = fpa[a] - *((float *)(&val));
			break;
		case 6: /* FPMUL to fpa[a] */
			fpa[a] = fpa[a] * *((float *)(&val));
			break;
		case 7: /* FPDIV into fpa[a] */
			fpa[a] = fpa[a] / *((float *)(&val));
			break;
		}
	}
}

WORD float_coget( int reg ) {
	/* coprocesor operation initiated by CPU */
	int a = reg & 1;
	int r = reg >> 1;
	cocc = 0; /* by default, we set no condition codes */
	if (!(costat & COFPENAB)) {
		return 0; /* floating point unit disabled */
	} else if (costat & FPLONG) { /* long version */
		int64_t bigval = *((int64_t *)(&fpa[a]));
		switch (r) {
		case 0: /* FPLOW -- context guarantes not COSTAT */
			return fplow;
		case 1: /* FPA[a] */
			fplow = (WORD)bigval; /* truncated to 32 bits */
			if (bigval < 0.0) cocc |= N;
			if (bigval == 0.0) cocc |= Z;
			if (!isfinite(fpa[a])) cocc |= C;
			return (WORD)(bigval >> 32);
		case 2: /* none */
		case 3: /* none */
		case 4: /* none */
		case 5: /* none */
		case 6: /* none */
		case 7: /* none */
			return 0;
		};
	} else { /* short version */
		float fval = fpa[a]; /* shorten operand */
		switch (r) {
		case 0: /* FPLOW -- context guarantes not COSTAT */
			return fplow;
		case 1: /* FPA[a] */
			if (fval < 0.0) cocc |= N;
			if (fval == 0.0) cocc |= Z;
			if (!isfinite(fval)) cocc |= C;
			return *((WORD *)(&fval));
		case 2: /* none */
		case 3: /* none */
		case 4: /* none */
		case 5: /* none */
		case 6: /* none */
		case 7: /* none */
			return 0;
		};
	}
}
