#ifndef UI
#define UI

#include "declarations.h"
#include "string_constants.h"
#include "quit_functions.h"
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <archive.h>
#include <archive_entry.h>

#define INFO_HEIGHT 3
#define SEL_COL 3
#define STAT_LENGTH 20
#define PERM_LENGTH 10

#define STATS_OFF 0
#define STATS_ON 1
#define STATS_IDLE 2

void screen_init(void);
void screen_end(void);
void reset_win(int win);
void new_tab(int win);
void change_first_tab_size(void);
void delete_tab(int win);
void scroll_down(void);
void scroll_up(void);
void trigger_show_helper_message(void);
void trigger_stats(void);
void change_unit(float size, char *str);
int win_getch(void);
void tab_refresh(int win);
#ifdef LIBUDEV_PRESENT
void update_devices(int num,  char (*str)[PATH_MAX]);
#endif
void print_info(const char *str, int i);
void ask_user(const char *str, char *input, int d, char c);
void resize_win(void);
void change_sort(void);
void highlight_selected(int line, const char c);
void erase_selected_highlight(void);

#endif