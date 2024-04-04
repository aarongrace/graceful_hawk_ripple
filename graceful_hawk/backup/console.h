/* File: console.h
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Nov. 7, 2019
   Purpose: Hawk Emulator console support interface;
*/

/* assumes prior inclusion of <stdint.h> and "bus.h" */

/*************************
 * memory mapped display *
 *************************/

void dispwrite(WORD addr, WORD val);
/* addr is relative to display's address range */
/* val is value to display */

WORD dispread(WORD addr);
/* addr is relative to display's address range */

/**************************
 * memory mapped keyboard *
 **************************/

void kbdwrite(WORD addr, WORD val);
/* addr is relative to keyboard's address range */
/* val is word to display */

WORD kbdread(WORD addr);
/* addr is relative to keyboard's address range */

/********************
 * console function *
 ********************/

void console_startup();
/* startup, called from main */

void console();
/* console, called from main when countdown < 0 or halt */
