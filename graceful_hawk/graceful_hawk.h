/*******************
 * Functions
 *******************/
void print_colorful_nums(uint32_t ints);
void change_theme(int theme);
void debug(uint32_t var);
void init_themes_and_color_pairs();
void switch_colorful_nums();
void set_banner_colors();
void set_banner_style(int incre);

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
#define p_theme_txt 41
#define p_memory_line 50
//the colors for numbers go from 80 to 95
#define p_nums 80

/*******************
 * Banner and theme constants
 *******************/
#define banner_temp 200
#define banner_len 6
#define bs_disco 0
#define bs_gradient 1
#define bs_alternating 2


#define t_default 1
#define t_desert 2
#define t_ocean 4
#define t_meadows 3
#define t_crimson 5
#define num_of_themes 5

/*******************
 * Global vars
 *******************/
extern char * theme_str;
extern int curr_theme;
extern bool cn_on;
extern int banner_stage;
extern int curr_banner_style;
extern char* curr_banner_char;
extern char* curr_banner_left;
extern char* curr_banner_right;

/*******************
 * Macros
 *******************/
#define printw_c(color_num, ...) \
	attron(COLOR_PAIR(color_num));\
	printw(__VA_ARGS__); \
	attroff(COLOR_PAIR(color_num));
