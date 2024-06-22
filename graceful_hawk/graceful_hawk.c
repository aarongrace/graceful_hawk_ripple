#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
/*#include <sys/ioctl.h>*/

#include "graceful_hawk.h"
#include "bus.h"


int curr_theme = t_default;
char * theme_str;
bool cn_on = false;
int banner_stage = 0;
int curr_banner_style = bs_gradient;
char* curr_banner_char;
char* curr_banner_left;
char* curr_banner_right;

static void init_themes();
void set_banner_style(int incre);
static void start_theme(int t);

/*******************
 * theme defs *
 *******************/

struct theme{
	int title[3];
	int status_text[3];
	int status_num[3];
	int register_text[3];
	int register_num[3];
	int memory_add[3];
	int memory_text[3];
	int memory_num[3];
	int menu[3];
	int cpu_line[3];
	int theme_txt[3];
	int memory_line[3];
	float n_mod0[3];
	float n_mod1[3];
	float n_mod2[3];
	float n_mod3[3];
};
typedef struct theme theme;

static theme themes[num_of_themes];

static float mr = 1;
static float mg = 1;
static float mb = 1;


void change_theme(int theme){
	if (theme == 0){
		curr_theme += 1;
		if (curr_theme > num_of_themes){
			curr_theme = t_default;
		}
	} else {
		curr_theme = theme;
	}
	start_theme(curr_theme);
}


void print_colorful_nums(uint32_t ints){

	for (int i = 7; i>= 0;i--){
		uint32_t temp_num = (ints >> 4 * i) &0xF;
		int color_index = temp_num + p_nums;
		attron(COLOR_PAIR(color_index));
		printw("%1X",temp_num);
		attroff(COLOR_PAIR(color_index));
	}
}

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


void set_banner_colors(){
	for (int i =0; i<banner_len; i++){
		int temp = banner_stage + i;
		if (temp >= banner_len){
			temp -= banner_len;
		}
		init_pair(banner_temp + i, banner_temp + temp, COLOR_BLACK);
	}
	banner_stage += 1;
	if (banner_stage >= banner_len){
		banner_stage = 0;
	}
}




/*************************
 * Graceful Hawk additions
 * ***********************/
WORD last_pc=0;
WORD last_jump;

#define debugy 6
#define debugx 1

void debug(WORD var){
	move(debugy, debugx);
	printw_c(p_menu,"dg:%08"PRIX32, var);
}
/*******************
 * colorful numbers for graceful hawk *
 *******************/

#define mod_colors_towards(end_r, end_g, end_b, steps) \
	mod_r += (end_r - mod_r)/steps; \
	mod_g += (end_g - mod_g)/steps; \
	mod_b += (end_b - mod_b)/steps; 


static void init_colorful_numbers_color_pairs(){
	theme t = themes[curr_theme];
	float mod_r= t.n_mod0[0];
	float mod_g= t.n_mod0[1];
	float mod_b= t.n_mod0[2];

	for (int i =0; i<16; i++){
		init_color(p_nums + i, mod_color_val(650, mod_r), mod_color_val(650, mod_g), mod_color_val(650, mod_b));
		init_pair(p_nums + i, COLOR_BLACK, p_nums + i);
		switch (i){
			case 0 ...5:
				mod_colors_towards( t.n_mod1[0], t.n_mod1[1], t.n_mod1[2], 6);
				break;
			case 6 ...11:
				mod_colors_towards( t.n_mod2[0], t.n_mod2[1], t.n_mod2[2], 6);
				break;

			case 12 ...15:
				mod_colors_towards( t.n_mod3[0], t.n_mod3[1], t.n_mod3[2], 4);

		}

	}
}
void switch_colorful_nums(){
	cn_on = !cn_on;
	//init_cn_color_pairs();
}


/*************************
 * main init function
 * ***********************/

void init_themes_and_color_pairs(){
	init_themes();
	start_theme(t_default);
}

static void start_theme(int t){
	switch (t){
		case t_desert:
			theme_str = "dune";
			mr = 1.18;
			mg = 1.05;
			mb = 0.91;
			break;
		case t_ocean:
			theme_str = "ocean";
			mr = 0.6;
			mg = 1.05;
			mb = 1.2;
			break;
		case t_crimson:
			theme_str = "crimson";
			mr = 1.35;
			mg = 0.75;
			mb = 0.7;
			break;
		case t_meadows:
			theme_str = "meadows";
			mr = 0.85;
			mg = 1.2;
			mb = 0.9;
			break;
		default:
			theme_str = "black marsh";
			mr = 1;
			mg = 1;
			mb = 1;
			break;
	}

	create_pair_bg_b(p_title, themes[t].title[0], themes[t].title[1], themes[t].title[2]);
	create_pair_bg_b(p_status_text, themes[t].status_text[0], themes[t].status_text[1], themes[t].status_text[2]);
	create_pair_bg_b(p_status_num, themes[t].status_num[0], themes[t].status_num[1], themes[t].status_num[2]);
	create_pair_bg_b(p_register_text, themes[t].register_text[0], themes[t].register_text[1], themes[t].register_text[2]);
	create_pair_bg_b(p_register_num, themes[t].register_num[0], themes[t].register_num[1], themes[t].register_num[2]);
	create_pair_bg_b(p_memory_add, themes[t].memory_add[0], themes[t].memory_add[1], themes[t].memory_add[2]);
	create_pair_bg_b(p_memory_text, themes[t].memory_text[0], themes[t].memory_text[1], themes[t].memory_text[2]);
	create_pair_bg_b(p_memory_num, themes[t].memory_num[0], themes[t].memory_num[1], themes[t].memory_num[2]);
	create_pair_bg_b(p_menu, themes[t].menu[0], themes[t].menu[1], themes[t].menu[2]);
	create_pair_bg_b(p_cpu_line, themes[t].cpu_line[0], themes[t].cpu_line[1], themes[t].cpu_line[2]);
	create_pair_bg_b(p_theme_txt, themes[t].theme_txt[0], themes[t].theme_txt[1], themes[t].theme_txt[2]);
	create_pair_bg_b(p_memory_line, themes[t].memory_line[0], themes[t].memory_line[1], themes[t].memory_line[2]);

	init_colorful_numbers_color_pairs();
	set_banner_style(0);
	set_banner_colors();
}

#define t_rgb(theme, r, g, b) \
	theme[0] = r; \
	theme[1] = g; \
	theme[2] = b; 

static void init_themes(){
	init_color(COLOR_BLACK, 110, 110, 100);
	t_rgb(themes[t_default].title,	 		650,	550,	950);
	t_rgb(themes[t_default].status_text,	800,	850,	500);
	t_rgb(themes[t_default].status_num,		700,	850,	960);
	t_rgb(themes[t_default].register_text,	1000,	850,	700);
	t_rgb(themes[t_default].register_num,	800,	750,	1000);
	t_rgb(themes[t_default].memory_add,		1000,	850,	700);
	t_rgb(themes[t_default].memory_text,	750,	950,	650);
	t_rgb(themes[t_default].memory_num,		650,	850,	950);
	t_rgb(themes[t_default].menu,			650,	670,	850);
	t_rgb(themes[t_default].cpu_line,		500,	650,	800);
	t_rgb(themes[t_default].theme_txt,		800,	900,	1000);
	t_rgb(themes[t_default].memory_line,	650,	450,	800);
	t_rgb(themes[t_default].n_mod0,			0.9,	1.5,	0.8);
	t_rgb(themes[t_default].n_mod1,			0.8,	1.1,	1.9);
	t_rgb(themes[t_default].n_mod2,			1.3,	0.9,	2.6);
	t_rgb(themes[t_default].n_mod3,			1.9,	1.2,	1.3);

	t_rgb(themes[t_desert].title,            767,  577,  864);
	t_rgb(themes[t_desert].status_text,      944,  892,  455);
	t_rgb(themes[t_desert].status_num,       826,  892,  873);
	t_rgb(themes[t_desert].register_text,   1000,  892,  637);
	t_rgb(themes[t_desert].register_num,     944,  787,  910);
	t_rgb(themes[t_desert].memory_add,      1000,  892,  637);
	t_rgb(themes[t_desert].memory_text,      885,  997,  591);
	t_rgb(themes[t_desert].memory_num,       767,  892,  864);
	t_rgb(themes[t_desert].menu,             767,  703,  773);
	t_rgb(themes[t_desert].cpu_line,         590,  782,  828);
	t_rgb(themes[t_desert].theme_txt,        944,  945,  410);
	t_rgb(themes[t_desert].memory_line,      767,  472,  728);
	t_rgb(themes[t_desert].n_mod0,			1.3,	0.8,	0.7);
	t_rgb(themes[t_desert].n_mod1,			1.5,	1.7,	0.4);
	t_rgb(themes[t_desert].n_mod2,			1.8,	1.1,	0.8);
	t_rgb(themes[t_desert].n_mod3,			0.5,	0.6,	1.6);

	t_rgb(themes[t_ocean].title,             390,  577, 1000);
	t_rgb(themes[t_ocean].status_text,       480,  892,  600);
	t_rgb(themes[t_ocean].status_num,        420,  892, 1000);
	t_rgb(themes[t_ocean].register_text,     600,  992,  840);
	t_rgb(themes[t_ocean].register_num,      480,  787, 1000);
	t_rgb(themes[t_ocean].memory_add,        600,  892,  840);
	t_rgb(themes[t_ocean].memory_text,       450,  997,  780);
	t_rgb(themes[t_ocean].memory_num,        390,  892, 1000);
	t_rgb(themes[t_ocean].menu,              390,  703, 1000);
	t_rgb(themes[t_ocean].cpu_line,          300,  682,  960);
	t_rgb(themes[t_ocean].theme_txt,         480,  945, 1000);
	t_rgb(themes[t_ocean].memory_line,       390,  472,  960);
	t_rgb(themes[t_ocean].n_mod0,			0.5,	1.2,	1.6);
	t_rgb(themes[t_ocean].n_mod1,			0.7,	1.9,	0.8);
	t_rgb(themes[t_ocean].n_mod2,			1.5,	0.5,	1.3);
	t_rgb(themes[t_ocean].n_mod3,			0.8,	1.3,	1.7);

	t_rgb(themes[t_crimson].title,           877,  412,  665);
	t_rgb(themes[t_crimson].status_text,    1000,  637,  350);
	t_rgb(themes[t_crimson].status_num,      745,  437,  872);
	t_rgb(themes[t_crimson].register_text,   900,  437,  889);
	t_rgb(themes[t_crimson].register_num,   1000,  562,  700);
	t_rgb(themes[t_crimson].memory_add,      700,  437,  889);
	t_rgb(themes[t_crimson].memory_text,     700,  412,  754);
	t_rgb(themes[t_crimson].memory_num,      877,  637,  665);
	t_rgb(themes[t_crimson].menu,            877,  502,  595);
	t_rgb(themes[t_crimson].cpu_line,        675,  487,  560);
	t_rgb(themes[t_crimson].theme_txt,      1000,  375,  800);
	t_rgb(themes[t_crimson].memory_line,     877,  337,  560);
	t_rgb(themes[t_crimson].n_mod0,			1.6,	1.2,	0.8);
	t_rgb(themes[t_crimson].n_mod1,			2.1,	0.8,	1.3);
	t_rgb(themes[t_crimson].n_mod2,			0.9,	1.5,	0.9);
	t_rgb(themes[t_crimson].n_mod3,			1.4,	0.6,	1.6);

	t_rgb(themes[t_meadows].title,           552,  660,  855);
	t_rgb(themes[t_meadows].status_text,     680, 1000,  450);
	t_rgb(themes[t_meadows].status_num,      595, 1000,  864);
	t_rgb(themes[t_meadows].register_text,   850, 1000,  630);
	t_rgb(themes[t_meadows].register_num,    680,  900,  900);
	t_rgb(themes[t_meadows].memory_add,      850, 1000,  630);
	t_rgb(themes[t_meadows].memory_text,     637, 1000,  585);
	t_rgb(themes[t_meadows].memory_num,      552, 1000,  855);
	t_rgb(themes[t_meadows].menu,            552,  804,  765);
	t_rgb(themes[t_meadows].cpu_line,        425,  780,  720);
	t_rgb(themes[t_meadows].theme_txt,       680, 1000,  900);
	t_rgb(themes[t_meadows].memory_line,     552,  540,  720);
	t_rgb(themes[t_meadows].n_mod0,			0.5,	1.8,	0.8);
	t_rgb(themes[t_meadows].n_mod1,			0.4,	0.7,	1.7);
	t_rgb(themes[t_meadows].n_mod2,			1.6,	1.4,	0.7);
	t_rgb(themes[t_meadows].n_mod3,			0.65,	1.9,	0.6);

}

void set_banner_style(int incre){
	curr_banner_style += incre;
	if (curr_banner_style > bs_alternating){
		curr_banner_style = bs_disco;
	}
	switch (curr_banner_style){
		case bs_disco:
			curr_banner_char = "$";
			curr_banner_left = "\\";
			curr_banner_right = "/";
			init_color(banner_temp + 0, mod_color_val(700, mr), mod_color_val(400, mg), mod_color_val(600, mb));
			init_color(banner_temp + 1, mod_color_val(400, mr), mod_color_val(800, mg), mod_color_val(500, mb));
			init_color(banner_temp + 2, mod_color_val(800, mr), mod_color_val(700, mg), mod_color_val(500, mb));
			init_color(banner_temp + 3, mod_color_val(500, mr), mod_color_val(400, mg), mod_color_val(800, mb));
			init_color(banner_temp + 4, mod_color_val(600, mr), mod_color_val(800, mg), mod_color_val(500, mb));
			init_color(banner_temp + 5, mod_color_val(700, mr), mod_color_val(600, mg), mod_color_val(850, mb));
			break;
		case bs_gradient:
			curr_banner_char = "~";
			curr_banner_left = "/";
			curr_banner_right = "\\";
			init_color(banner_temp + 0, mod_color_val(1000, mr), mod_color_val(800, mg), mod_color_val(550, mb));
			init_color(banner_temp + 1, mod_color_val(800, mr), mod_color_val(850, mg), mod_color_val(750, mb));
			init_color(banner_temp + 2, mod_color_val(775, mr), mod_color_val(700, mg), mod_color_val(850, mb));
			init_color(banner_temp + 3, mod_color_val(500, mr), mod_color_val(700, mg), mod_color_val(1000, mb));
			init_color(banner_temp + 4, mod_color_val(750, mr), mod_color_val(800, mg), mod_color_val(900, mb));
			init_color(banner_temp + 5, mod_color_val(800, mr), mod_color_val(600, mg), mod_color_val(950, mb));
			break;
		case bs_alternating:
			curr_banner_char = "|";
			curr_banner_left = "-";
			curr_banner_right = "-";
			init_color(banner_temp + 0, mod_color_val(600, mr), mod_color_val(700, mg), mod_color_val(900, mb));
			init_color(banner_temp + 1, mod_color_val(600, mr), mod_color_val(700, mg), mod_color_val(900, mb));
			init_color(banner_temp + 2, mod_color_val(600, mr), mod_color_val(700, mg), mod_color_val(900, mb));
			init_color(banner_temp + 3, mod_color_val(900, mr), mod_color_val(600, mg), mod_color_val(700, mb));
			init_color(banner_temp + 4, mod_color_val(900, mr), mod_color_val(600, mg), mod_color_val(700, mb));
			init_color(banner_temp + 5, mod_color_val(900, mr), mod_color_val(600, mg), mod_color_val(700, mb));
			break;
	}
}
