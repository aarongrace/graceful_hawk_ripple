/* File: float.h
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Aug. 21, 2011
   Revised:  Nov. 8, 2023 - added float_acc for front panel display

   Language: C (UNIX)
   Purpose: Hawk floating point coprocessor interface definitions 
*/

/* assumes prior inclusion of <stdint.h> and "bus.h" */

double float_acc( int i );
        /* read-only access to floating point accumulators for front panel */

void float_coset( int reg, WORD val );
        /* coprocesor operation initiated by CPU */

WORD float_coget( int reg );
        /* coprocesor operation initiated by CPU */
