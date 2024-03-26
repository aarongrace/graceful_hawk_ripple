/* File: powerup.c
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Mar. 6, 1996
   Revised: Nov. 9, 2023 - (WORD)casting, -Z command line arg, error msgs
   Language: C (UNIX)
   Purpose: Hawk Emulator Power-On support;
		parses command line arguments and loads object file.
*/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "bus.h"
#include "powerup.h"

static FILE *f = NULL;

/**************************************
 * error diagnostic output for loader *
 **************************************/

static void diagnose(int c) {
	/* put c to stderr for diagnostic */
	if (c < 0) {
		fputs("EOF", stderr);
	} else if (c < ' ') {
		putc('^', stderr);
		putc(c + '@', stderr);
	} else if (c > 0x7F) {
		putc('+', stderr);
		putc(c - 0x7F, stderr);
	} else {
		putc(c, stderr);
	}
}

static void wipeout() {
	fputs(" in object file **\n", stderr);
	exit(EXIT_FAILURE); /* error */
}

/**********
 * loader *
 **********/

/* documentation for the syntax of the SMAL loader sublanguage is found in
 * section 9.1 of the SMAL manual:
 * https://homepage.cs.uiowa.edu/~dwjones/cross/smal32/loader.html#load
 */

static WORD lc = 0UL; /* the location counter */
static WORD rb = 0UL; /* the relocation base */

static void getcheck(char c) {
	/* get char from SMAL32 object file and verify that it's c */
	int ch = getc(f);
	if (ch == 0x0D){
		ch = getc(f);
	}
	if (ch != c) {
		fputs("** found '", stderr);
		diagnose(c);
		fputs("' where '", stderr);
		diagnose(ch);
		fputs("' expected", stderr);
		wipeout();
	}
	
}

static WORD load_value() {
	/* parse a load value from SMAL32 object file, up through EOL */
	int ch = getc(f);
	if (ch == '#') {
		WORD value = 0UL;
		ch = getc(f);
		do {
			if ((ch >= '0')&&(ch <= '9')) {
				value = (value << 4) | (ch - '0');
			} else if ((ch >= 'A')&&(ch <= 'F')) {
				value = (value << 4) | (ch - ('A' - 10));
			} else {
				fputs("** found '", stderr);
				diagnose(ch);
				fputs("' where hex digit expected", stderr);
				wipeout();
			}
			ch = getc(f);
		} while ((ch != '\n') && (ch != '+') && (ch != 0x0D));
		if (ch == '+') {
			getcheck('R');
			getcheck('\n');
			value += rb;
		} else if (ch != '\n') {
			fputs("** found '", stderr);
			diagnose(ch);
			fputs("' where EOL expected", stderr);
			wipeout();
		}
		return value;
	} else if (ch == ' ') {
		getcheck('R');
		getcheck('\n');
		return rb;
	} else {
		fputs("** found '", stderr);
		diagnose(ch);
		fputs("' where load value expected", stderr);
		wipeout();
	}
}

static void storebyte(WORD loc, WORD val) {
	/* store BYTE val in m[loc] */
	WORD a = loc >> 2;
	int s = (loc & (WORD)0x00000003UL) << 3; /* shift count within word */
	WORD w;
	if (loc >= MAXMEM) {
		fputs("** invalid load address", stderr);
		wipeout();
	}
	w = m[a];
	w = (w & ~((WORD)0x000000FFUL << s))|((val & (WORD)0x000000FFUL) << s);
	m[a] = w;
}

static void load() {
	/* load a SMAL32 object file */
	int ch;
	ch = getc(f);
	while (ch != EOF) {
		if (ch == 'W') {
			WORD w = load_value();
			storebyte(lc,     w      );
			storebyte(lc + 1, w >>  8);
			storebyte(lc + 2, w >> 16);
			storebyte(lc + 3, w >> 24);
			lc += 4;
		} else if (ch == 'T') {
			WORD t = load_value();
			storebyte(lc,     t      );
			storebyte(lc + 1, t >>  8);
			storebyte(lc + 2, t >> 16);
			lc += 3;
		} else if (ch == 'H') {
			WORD h = load_value();
			storebyte(lc,     h      );
			storebyte(lc + 1, h >>  8);
			lc += 2;
		} else if (ch == 'B') {
			WORD b = load_value();
			storebyte(lc,     b      );
			lc += 1;
		} else if (ch == '.') {
			getcheck('=');
			lc = load_value();
		} else if (ch == 'R') {
			getcheck('=');
			getcheck('.');
			getcheck('\n');
			rb = lc;
		} else if (ch == 'S') {
			breakpoint = load_value();
			if (pc & (WORD)0x00000001UL) {
				fputs("** odd start address", stderr);
				wipeout();
			}
		} else {
			fputs("** found '", stderr);
			diagnose(ch);
			fputs("' where load directive expected", stderr);
			wipeout();
		}
		ch = getc(f);
	}
}

void powerup(int argc, char **argv) {
	int i;
	recycle = 20; /* by default update console display every 20 mem refs */

	for (i = 1; i < argc; i++) { /* for each argument */
		if (argv[i][0] == '-') {
			if ((argv[i][1] == 'Z')&&(argv[i][2] == '\0')) {
				i++;
				if (i < argc) {
					char * e;
					recycle == (WORD)strtol(argv[i],&e,10);
					if ((e == argv[i]) || (*e != '\0')){
						fputs(argv[0], stderr);
						fputs(" -Z ", stderr);
						fputs(argv[i], stderr);
						fputs(": bad number\n", stderr);
						exit(EXIT_FAILURE); /* error */
					}
				} else {
					fputs(argv[0], stderr);
					fputs(" -Z", stderr);
					fputs(": missing sleep time\n", stderr);
					exit(EXIT_FAILURE); /* error */
				}
			} else if ((argv[i][1] == '?')&&(argv[i][2] == '\0')) {
				fputs(argv[0], stderr);
				fputs(" [-Z cycles] load file list\n", stderr);
				exit(EXIT_SUCCESS); /* error */
			} else {
				fputs(argv[0], stderr);
				fputs(" ", stderr);
				fputs(argv[i], stderr);
				fputs(": bad command line option\n", stderr);
				exit(EXIT_FAILURE); /* error */
			}
                } else {
			f = fopen(argv[i], "r");
			if (f == NULL) {
				fputs(argv[0], stderr);
				fputs(" ", stderr);
				fputs(argv[i], stderr);
				fputs(": cannot open object file\n", stderr);
				exit(EXIT_FAILURE); /* error */
			}
			load();
			fclose(f);
			f = NULL;
		}
	}
}
