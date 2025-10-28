/* File: showop.h
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Nov. 7, 2019

   Language: C (UNIX) with -lcurses option
   Purpose: Hawk Emulator, interface to disassembler for HAWK opcodes
*/

/* assumes prior inclusion of <stdint.h> and "bus.h" */

/*******************************
 * disassemble one instruction * 
 *******************************/

int showop( WORD a );
/* decode the opcode in m[a] and output it; returns address increment */

int sizeofop( WORD a );
/* decode the opcode in m[a] and return address increment */
