#ifndef UI
#define UI

#include "declarations.h"
#include "string_constants.h"
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <archive.h>
#include <archive_entry.h>

#define INFO_HEIGHT 2
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

#define STATS_OFF 0
#define STATS_ON 1
#define STATS_IDLE 2

void screen_init(void);
void screen_end(void);
int sizesort(const struct dirent **d1, const struct dirent **d2);
int last_mod_sort(const struct dirent **d1, const struct dirent **d2);
void reset_win(int win);
void list_everything(int win, int old_dim, int end);
void new_tab(int win);
void restrict_first_tab(void);
void delete_tab(int win);
void enlarge_first_tab(void);
void scroll_down(void);
void scroll_up(void);
void trigger_show_helper_message(void);
void trigger_stats(void);
int win_getch(void);
void tab_refresh(int win);
void print_info(const char *str, int i);
void ask_user(const char *str, char *input, int dim, char c);
void resize_win(void);
void change_sort(void);

#endif