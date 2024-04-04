/* File: console.c
   Author: Douglas Jones, Dept. of Comp. Sci., U. of Iowa, Iowa City, IA 52242.
   Date: Mar. 6, 1996
   Revised: Feb. 27, 2002 - replace BCDGET with EX3ADJ instruction
   Revised: Mar.  1, 2002 - add SSQADJ
   Revised: Mar. 13, 2002 - add BTRUNC
   Revised: Apr. 23, 2002 - strip out showop() into showop.c
   Revised: Aug. 24, 2008 - flip byte order in IR, add n console command
   Revised: Nov.  8, 2023 - added display of costat, fp accumulators
   Revised: Dec. 11, 2023 - make interrupts work, make polling KBDSTAT polite

   Language: C (UNIX) with -lcurses option
   Purpose: Hawk Emulator console support;
*/
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
/*#include <sys/ioctl.h>*/

#include "bus.h"
#include "float.h"
#include "showop.h"
#include "console.h"

/*****************
 * screen layout *
 *****************/

/* title location */
#define titley 1
#define titlex 5

/* number location */
#define numbery 1
#define numberx 36

/* pc location */
#define pcy 3
#define pcx 3

/* dump location */
#define dumpy 3
#define dumpx 45

/* menu location */
#define menuy 12
#define menux 1

/* memory mapped display location */
#define dispy 14
#define dispx 0

/*********************
 * memory addressing *
 *********************/

/* relative memory addresses of memory mapped output display;
   these are relative to DISPBASE the start address for the entire display
   and apply only within the procedures dispwrite and dispread.
 */
#define DISPLINES  0
#define DISPCOLS   4
#define DISPSTART  0x100

/* relative memory addresses of memory mapped keyboard interface;
   these are relative to KBDBASE the start address for the entire keyboard
   and apply only within the procedures kbdwrite and kbdread.
*/
#define KBDDATA  0
#define KBDSTAT  4


/********************
 * console controls *
 ********************/

/* basic machine state */
int running = FALSE;
int broken = FALSE;

/* cycle count value seen at time of break */
WORD breakcycles = 0;

/* number being entered */
WORD number = 0;

/* dump controls */
WORD dump_addr = 0;
#define DATAMODE 0
#define CODEMODE 1
WORD dump_mode = CODEMODE;
/* menu to display */
#define NUM_MENUS 7
	int which_menu = 1;

/*************************
 * Symbolic Dump Support *
 *************************/


/*******************
 * Graceful Hawk colors *
 *******************/
/* starts at 30*/
#define p_title 31
#define p_status_text 32
#define p_status_num 33
#define p_register_text 34
#define p_register_num 35
#define p_memory_add 36
#define p_memory_text 37
#define p_memory_num 38
#define p_menu 39
#define p_cpu_line 40
#define p_theme 41
#define p_memory_line 50

#define p_banner 60
#define banner_temp 200
#define banner_len 6
static int banner_stage = 0;

#define t_default 1
#define t_desert 2
#define t_ocean 4
#define t_meadows 3
#define t_crimson 5
#define t_last 5
static int curr_theme = t_default;
static char * theme_str;

static void create_pair_bg_b(int index, int r, int g, int b){
	init_color(index, r, g, b);
	init_pair(index, index, COLOR_BLACK);
}

static int mod_color_val(int color, float m){
	int new_color = (int) color * m;
	//return the minimum of the new color and 1000
	if (new_color>1000){
		return 1000;
	} else {
		return new_color;
	}
}
static float mr = 1;
static float mg = 1;
static float mb = 1;
static void set_banner_colors(int stage){
	for (int i =0; i<banner_len; i++){
		int temp = stage + i;
		if (temp >= banner_len){
			temp -= banner_len;
		}
		init_pair(banner_temp + 1, banner_temp + temp, COLOR_BLACK);
	}
}
static void set_theme(){
	switch (curr_theme){
		case t_desert:
			theme_str = "DUNE";
			mr = 1.18;
			mg = 1.05;
			mb = 0.91;
			break;
		case t_ocean:
			theme_str = "OCEAN";
			mr = 0.6;
			mg = 1.05;
			mb = 1.2;
			break;
		case t_crimson:
			theme_str = "CRIMSON";
			mr = 1.35;
			mg = 0.75;
			mb = 0.7;
			break;
		case t_meadows:
			theme_str = "MEADOWS";
			mr = 0.85;
			mg = 1.2;
			mb = 0.9;
			break;
		default:
			theme_str = "DEFAULT";
			break;
	}
	create_pair_bg_b(p_title, mod_color_val(650, mr),mod_color_val(550, mg),mod_color_val(950, mb));
	create_pair_bg_b(p_status_text, mod_color_val(800, mr),mod_color_val(850, mg),mod_color_val(500, mb));
	create_pair_bg_b(p_status_num, mod_color_val(700, mr),mod_color_val(850, mg),mod_color_val(960, mb));
	create_pair_bg_b(p_register_text, mod_color_val(1000, mr),mod_color_val(850, mg),mod_color_val(700, mb));
	create_pair_bg_b(p_register_num, mod_color_val(800, mr),mod_color_val(750, mg),mod_color_val(1000, mb));
	create_pair_bg_b(p_memory_add, mod_color_val(1000, mr),mod_color_val(850, mg),mod_color_val(700, mb));
	create_pair_bg_b(p_memory_text, mod_color_val(750, mr),mod_color_val(950, mg),mod_color_val(650, mb));
	create_pair_bg_b(p_memory_num, mod_color_val(650, mr),mod_color_val(850, mg),mod_color_val(950, mb));
	create_pair_bg_b(p_menu, mod_color_val(650, mr),mod_color_val(670, mg),mod_color_val(850, mb));
	create_pair_bg_b(p_cpu_line, mod_color_val(500, mr),mod_color_val(650, mg),mod_color_val(800, mb));
	create_pair_bg_b(p_theme, mod_color_val(800, mr),mod_color_val(900, mg),mod_color_val(1000, mb));
	create_pair_bg_b(p_memory_line, mod_color_val(650, mr),mod_color_val(450, mg),mod_color_val(800, mb));

	init_color(banner_temp + 0, mod_color_val(700, mr),mod_color_val(400, mg),mod_color_val(600, mb));
	init_color(banner_temp + 1, mod_color_val(400, mr),mod_color_val(800, mg),mod_color_val(500, mb));
	init_color(banner_temp + 2, mod_color_val(800, mr),mod_color_val(700, mg),mod_color_val(500, mb));
	init_color(banner_temp + 3, mod_color_val(500, mr),mod_color_val(400, mg),mod_color_val(800, mb));
	init_color(banner_temp + 4, mod_color_val(600, mr),mod_color_val(800, mg),mod_color_val(500, mb));
	init_color(banner_temp + 0, mod_color_val(700, mr),mod_color_val(600, mg),mod_color_val(850, mb));
	set_banner_colors(banner_stage);
}

static void change_theme(int theme){
	if (theme == 0){
		curr_theme += 1;
		if (curr_theme > t_last){
			curr_theme = t_default;
		}
	} else {
		curr_theme = theme;
	}
	set_theme();
}

#define printw_c(color_num, ...) \
	attron(COLOR_PAIR(color_num));\
	printw(__VA_ARGS__); \
	attroff(COLOR_PAIR(color_num));

/*************************
 * Graceful Hawk additions
 * ***********************/
WORD last_pc=0;
WORD last_jump;

#define debugy 6
#define debugx 1

static void debug(WORD var){
	move(debugy, debugx);
	printw_c(p_menu,"debug:  %08"PRIX32, var);
}

/*******************
 * console display *
 *******************/
static int banner_at = 0;
static void print_banner_char(char* c, int reps){
	for (int i; i < reps; i++){
		int temp = banner_at % banner_len;
		printw_c(banner_temp + temp, c);
		banner_at++;
	}
}

static void title() {
	/* put title on screen */
	move(titley, titlex);
	printw_c(p_title,"The Graceful Hawk Emulator");
	move(pcy - 1, pcx);

	print_banner_char("/", 1);
	print_banner_char("-", 4);
	//the cpu line should be 43 chars long 
	printw_c(p_cpu_line,"CPU");
	print_banner_char("-", 4);
	printw_c(p_cpu_line,"THEME:");
	printw_c(p_theme, theme_str);
	print_banner_char("-",22-strlen(theme_str));
	print_banner_char("\\", 1);

	if (COLS < (dumpx + 18)) return; /* no space on screen */
	move(dumpy - 1, dumpx);
	printw_c(p_memory_line, "  /----MEMORY----\\");
}


static void status() {
	/* display CPU status on screen */
	char n,z,v,c;
	int i;
	n = z = v = c = '0';
	if (psw & N) n = '1'; 
	if (psw & Z) z = '1'; 
	if (psw & V) v = '1'; 
	if (psw & C) c = '1'; 
	move(pcy, pcx);                             /* 0123456789012345 */
	printw_c(p_status_text, "PC:  ");               /* PC:  00000000 */
	printw_c(p_status_num, "%08"PRIX32, pc);               /* PC:  00000000 */
	move(pcy + 1, pcx);
	printw_c(p_status_text, "PSW: ");              /* PSW: 00000000 */
	printw_c(p_status_num, "%08"PRIX32, psw);              /* PSW: 00000000 */
	move(pcy + 2, pcx);
	printw_c(p_status_text, "NZVC: ");    /* NZVC: 0 0 0 0 */
	printw_c(p_status_num, "%c %c %c %c", n, z, v, c);    /* NZVC: 0 0 0 0 */
	if (costat & COENAB) {
		move(pcy + 4, pcx);
		printw_c(p_status_text, "COSTAT: ");/* COSTAT: 0000  */
		printw_c(p_status_num, "%04"PRIX32, costat);/* COSTAT: 0000  */
	} else {
		move(pcy + 4, pcx);
		printw("             ");            /*     erase     */
	}
	if (costat & COFPENAB) {
		move(pcy + 5, pcx);
		printw_c(p_status_text, "/----FPU----\\");           /* /----FPU----\ */
		move(pcy + 6, pcx);
		printw("A0: %9.3g", float_acc(0));  /* A0: 0.000E000 */
		move(pcy + 7, pcx);
		printw("A1: %9.3g", float_acc(1));  /* A1: 0.000E000 */
	} else {
		move(pcy + 5, pcx);
		printw("             ");            /*     erase     */
		move(pcy + 6, pcx);
		printw("             ");
		move(pcy + 7, pcx);
		printw("             ");
	}
	for (i = 1; i < 8; i++) {
		move(pcy + i, pcx + 15);
		printw_c(p_register_text, "R%1X: ", i);
		printw_c(p_register_num, "%08"PRIX32, r[i]);
	}
	for (i = 8; i < 16; i++) {
		move(pcy + (i - 8), pcx + 29);
		printw_c(p_register_text, "R%1X: ", i);
		printw_c(p_register_num, "%08"PRIX32, r[i]);
	}
}

static void dump() {
	/* display memory on screen */
        unsigned int i;
	if (COLS < (dumpx + 18)) return; /* no space on screen */
	if (dump_mode == DATAMODE) { /* do a hex dump */
		dump_addr &= 0x00FFFFFCUL;
		for (i = 0; i < 8; i += 1) {
			WORD addr = dump_addr + (i<<2);
			move(dumpy + i, dumpx);
			if (addr == (pc & 0xFFFFFFFCUL)) {
				if (addr == (breakpoint & 0xFFFFFFFCUL)) {
					addstr("-*");
				} else {
					addstr("->");
				}
			} else {
				if (addr == (breakpoint & (WORD)0xFFFFFFFCUL)) {
					addstr(" *");
				} else {
					addstr("  ");
				}
			}
			if (addr < MAXMEM) {
				WORD data = m[addr>>2];
				printw_c(p_memory_add, "%06"PRIX32": ", addr&(WORD)0x00FFFFFFUL)
				printw_c(p_memory_num, "%08"PRIX32, data);
				if (COLS >= (dumpx + 25)) {
					int i;
					attron(COLOR_PAIR(p_memory_text));
					addch(' ');
					for (i = 0; i < 4; i++) {
						char ch = (data>>(i<<3)) & 0x7F;
						if (ch < ' ') ch = ' ';
						addch(ch);
					}
					attroff(COLOR_PAIR(p_memory_text));
				}
			} else {
				printw_c(p_memory_add, "%06"PRIX32": ", addr&(WORD)0x00FFFFFFUL)
				printw_c(p_memory_num, "--------");
			}
			clrtoeol();
		}
        } else { /* dump_mode == CODEMODE */
		WORD addr;
		int pcnotseen = TRUE;
		int trial = 0;
		do {
			addr = dump_addr + trial;
			for (i = 0; i < 8; i += 1) {
				addr &= (WORD)0x00FFFFFEUL;
				move(dumpy + i, dumpx);
				if (addr == pc) {
					if (addr == breakpoint) {
						addstr("-*");
					} else {
						addstr("->");
					}
					pcnotseen = FALSE;
				} else {
					if (addr == breakpoint) {
						addstr(" *");
					} else {
						addstr("  ");
					}
				}
				if (addr < MAXMEM) {
					printw_c(p_memory_add, "%06" PRIX32 ": ", addr & (WORD)0x00FFFFFFUL);
					attron(COLOR_PAIR(p_memory_text));
					addr += showop(addr); 
					attroff(COLOR_PAIR(p_memory_text));
				} else {
					printw("%06"PRIX32": --",
						addr&(WORD)0x00FFFFFFUL);
					addr += 2; 
				}
				clrtoeol();
			}
			trial += 2;
		} while (((dump_addr+trial) <= pc) && (pc < addr) && pcnotseen);
	}
}

static void shownum() {
	/* display the accumulated number */
	move(numbery, numberx);
	if (number == 0) {
		addstr("        ");
	} else {
		printw("%08"PRIX32, number);
	}
}

static void menu() {
	/* display console status */
	static char * menus[]= {
		"  RUNNING   control c - halt",
		"**HALTED**  r s q ? o(step out) v(change theme)",
		"**HALTED**  0-9/A-F(enter n) m(show m[n])"
			" +-(adjust n) ?(help)",
		"**HALTED**  t(toggle memory display) ?(help)",
		"**HALTED**  0-9/A-F(enter n) p(run until pc=n)"
			" <>(adjust n) ?(help)",
		"**HALTED**  n(next)"
			" i(iterate) ?(help)",
		"**HALTED**  0-9/A-F(enter n)"
			" z(set refresh interval=n) ?(help)"
	};
	move(menuy, menux);
	if (running) {
		which_menu = 0;
	}
	attron(COLOR_PAIR(p_menu));
	addstr(menus[which_menu]);
	attroff(COLOR_PAIR(p_menu));
	clrtoeol();
	refresh();
}

/********************************
 * memory mapped output display *
 ********************************/

static int dispend; /* the end address of the display memory */
static int dispcols; /* the number of displayed columns */

void dispwrite(WORD addr, WORD val) {
	/* addr is relative to display's address range */
	/* val is value to display */
	if (addr >= (DISPBASE + DISPSTART)) {
		if (addr >= dispend) { 
			return;
		} else {
			int relad = addr - (DISPBASE + DISPSTART);
			int x = dispx + relad % dispcols;
			int y = dispy + relad / dispcols;
			char c0 =  val       & 0x7F;
			char c1 = (val >> 8) & 0x7F;
			char c2 = (val >>16) & 0x7F;
			char c3 = (val >>24) & 0x7F;
			move(y, x);
			if (c0 >= ' ') {addch(c0);} else {addch(c0|'@');}
			if (c1 >= ' ') {addch(c1);} else {addch(c1|'@');}
			if (c2 >= ' ') {addch(c2);} else {addch(c2|'@');}
			if (c3 >= ' ') {addch(c3);} else {addch(c3|'@');}
			return;
		}
	} else {
		return;
	}
}

WORD dispread(WORD addr) {
	/* addr is relative to display's address range */
	if (addr >= (DISPBASE + DISPSTART)) {
		if (addr >= dispend) { 
			return 0xFFFFFFFF;
		} else {
			int relad = addr - (DISPBASE + DISPSTART);
			int x = dispx + relad % dispcols;
			int y = dispy + relad / dispcols;
			unsigned char c0,c1,c2,c3;
			move(y, x);
			c0 = inch();
			move(y, x+1);
			c1 = inch();
			move(y, x+2);
			c2 = inch();
			move(y, x+3);
			c3 = inch();
			return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
		}
	} else if (addr == (DISPBASE + DISPLINES)) {
		return (LINES - dispy) - 1;
	} else if (addr == (DISPBASE + DISPCOLS)) {
		return COLS;
	} else {
		return 0xFFFFFFFF;
	}
}


/**************************
 * memory mapped keyboard *
 **************************/

static BYTE kbdbuf = 0;    /* most recent character read from keyboard */
static BYTE kbdstat = 0;   /* keyboard status bits follow */

/* bits in kbdstat, interrupt enable, error and ready */
#define KBDIE    0x80
#define KBDERR   0x40
#define KBDRDY   0x01

/* keyboard interrupt at level 7 in irq register */
#define KBDIRQ   IRQ7

static void kbdpoll() {
	/* call whenever there is a need to poll the keyboard for input */
	int ch;
	ch = getch();
	if (ch == ERR) return;
	kbdbuf = (BYTE)ch;
	if ((kbdstat & KBDRDY) == 0) {
		kbdstat |= KBDRDY;
	} else {
		/* overrun error */
		kbdstat |= KBDERR;
	}
	if ((kbdstat & KBDIE) != 0) {
		irq |= KBDIRQ;
	}
}

void kbdwrite(WORD addr, WORD val) {
	/* addr is relative to keyboard's address range */
	/* val is word to display */
	if (addr == (KBDBASE + KBDDATA)) {
		/* no obvious function */
	} else if (addr == (KBDBASE + KBDSTAT)) {
		kbdstat &= ~(KBDIE | KBDERR);	/* turn off  IE & ERR bits */
		val     &=  (KBDIE | KBDERR);   /* keep only IE & ERR bits */
		kbdstat |= ((BYTE)val);
		/* only IE and ERR change, all else unchanged */
	}
	if ((kbdstat & KBDIE) == 0) { /* if interrupts disabled */
		irq &= ~KBDIRQ; /* retract interrupt request */
	}
}

WORD kbdread(WORD addr) {
	/* addr is relative to keyboard's address range */
	if (addr == (KBDBASE + KBDDATA)) {
		kbdstat &= ~KBDRDY; /* turn off ready bit */
		irq &= ~KBDIRQ; /* retract interrupt request */
		return (WORD)kbdbuf;
	} else if (addr == (KBDBASE + KBDSTAT)) {
		WORD retval = (WORD)kbdstat;
		if (!(retval & KBDRDY)) { /* if keyboard not ready */
			usleep( 50000 );  /* be polite, 0.05 second delay */
		}			  /* so polling loops relinquish cpu */
		morecycles += cycles; /* be nice ...  */
		cycles = 0;	      /* let output echo and keyboard poll */
		return (WORD)retval;
	}
}


/********************
 * console function *
 ********************/

static void console_stop() {
	/* shutdown */
	signal(SIGINT, SIG_IGN);
	mvcur(0, COLS-1, LINES-1, 0);
	endwin(); /* curses wrapup */
	exit(EXIT_SUCCESS);
}

static void console_sig(int sigraised) {
	/* control C or other events that stop run */
	/* sigraised is ignored! */
	signal(SIGINT, console_sig);
	breakcycles = cycles;
	cycles = 0;
	running = FALSE;
	broken = TRUE;
}

void console_startup() {
	/* startup, called from main */
	/* initializes color themes*/
	/* assume that breakpoint is already set or zeroed */
	initscr(); cbreak(); noecho(); clear(); /* curses startup */
	start_color();
	set_theme();
	signal(SIGINT, console_sig);
	title();
	menu();

	dispcols = COLS - dispx;
	dispend = DISPBASE + (DISPSTART + (((LINES - dispy)-1) * (dispcols)));
}

void console() {
	/* console, called from main when countdown < 0 or halt */
	if (dump_mode == CODEMODE) {
		if (dump_addr > (pc + 16)) {
			dump_addr = pc - 4;
		} else if (dump_addr > pc ) {
			dump_addr = pc;
		} else if ((dump_addr + 16) <= pc) {
			dump_addr = pc - 4;
		}
	}
	dump();
	status();
	debug(r[1]);
	if ((pc == breakpoint)||(pc == 0)) { /* address zero always a break */
		running = FALSE;
		which_menu = 1;
	}
	if (running) {
		cycles -= recycle;
		morecycles += recycle;
		kbdpoll();
		refresh();
		return;
	}
	if (broken) {
		cycles = breakcycles;
		which_menu = 1;
		touchwin(stdscr);
		refresh();
		broken = FALSE;
	}
	menu();
	for (;;) {
		int ch;
		nodelay(stdscr, FALSE);
		ch = getch();

		/* if the character is a digit, accumulate it */
		if ((ch >= '0') && (ch <= '9')) {
			number = (number << 4) | (ch & 0xF);
			shownum();
			refresh();
		} else if ((ch >= 'A') && (ch <= 'F')) {
			number = (number << 4) | (ch + (10 - 'A'));
			shownum();
			refresh();
		} else if ((ch >= 'a') && (ch <= 'f')) {
			number = (number << 4) | (ch + (10 - 'a'));
			shownum();
			refresh();
		} else switch (ch) { /* else it's not a digit */

		case 'r': /* run command */
			running = TRUE;
			menu();
			morecycles += (recycle + cycles);
			cycles = -recycle; /* next refresh when it's positive */
			nodelay(stdscr, TRUE);
			return;

		case 's': /* single step command */
			morecycles += cycles;
			cycles = 0;
			nodelay(stdscr, TRUE);
			return;

		case 'p': /* set breakpoint = number and run command */
			breakpoint = number & 0xFFFFFFFEUL;
			running = TRUE;
			number = 0;
			shownum();
			menu();
			refresh();
			morecycles += (recycle + cycles);
			cycles = -recycle;
			nodelay(stdscr, TRUE);
			return;

		case 'i': /* set breakpoint = pc and run command */
			breakpoint = pc;
			running = TRUE;
			menu();
			morecycles += (cycles+recycle);
			cycles = -recycle;
			nodelay(stdscr, TRUE);
			return;

		case 'o': /* set breakpoint = lastjump and run command*/
			breakpoint = r[1];
			running = TRUE;
			menu();
			morecycles += (cycles+recycle);
			cycles = -recycle;
			nodelay(stdscr, TRUE);
			return;

		case 'n': /* set breakpoint = next instr and run command */
			breakpoint = pc + sizeofop(pc);
			running = TRUE;
			menu();
			morecycles += (cycles+recycle);
			cycles = -recycle;
			nodelay(stdscr, TRUE);
			return;

		case '>': /* move breakpoint */
			breakpoint += 2;
			dump();
			refresh();
			break;

		case '<': /* move breakpoint */
			breakpoint -= 2;
			dump();
			refresh();
			break;

		case 'z': /* set execution speed */
			if ((number > 0)&&(number <= 131072)) {
				recycle = number;
				number = 0;
				shownum();
				refresh();
			}
			break;

		case 'q': /* quit command */
			console_stop();

		case 'm': /* memory dump command */
			dump_addr = number;
			number = 0;
			shownum();
			dump();
			refresh();
			break;

		case 't': /* toggle memory dump mode command */
			if (dump_mode == DATAMODE) {
				dump_mode = CODEMODE;
			} else {
				dump_mode = DATAMODE;
			}
			dump();
			refresh();
			break;
		
		case 'v': /* switch theme*/
			change_theme(0);
			title();
			refresh();
			break;

		case '+': /* increment dumped memory command */
			if (dump_mode == DATAMODE) {
				dump_addr += 0x0010UL;
			} else {
				dump_addr += 0x0008UL;
			}
			dump();
			refresh();
			break;

		case '-': /* decrement dumped memory command */
			if (dump_mode == DATAMODE) {
				dump_addr -= 0x0010UL;
			} else {
				dump_addr -= 0x0008UL;
			}
			dump();
			refresh();
			break;

		case '?': /* help command */
			which_menu++;
			if (which_menu >= NUM_MENUS) which_menu = 1;
			menu();
			break;
		}
	}
}
