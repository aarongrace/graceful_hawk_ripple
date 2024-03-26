/* File: irfields.h
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Dec 2007
   Language: C (UNIX)
   Purpose: Declarations of fields of the instruction register
   Constraints: When included, ir must be defined.
*/

/* ir fields |S1 S2 OP DST| or |CONST OP DST| */
#define OP    ((ir >>  4) & 0xF)
#define DST   ((ir      ) & 0xF)
#define S1    ((ir >> 12) & 0xF)
#define OP1   S1
#define S2    ((ir >>  8) & 0xF)
#define SRC   S2
#define X     S2
#define OP2   S2
#define CONST ((ir >>  8) & 0xFF)
