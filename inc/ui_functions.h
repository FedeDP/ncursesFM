#ifndef UI
#define UI

#include "declarations.h"
#include "string_constants.h"
#include <locale.h>
#include <ncurses.h>
#include <signal.h>

#define INFO_HEIGHT 2
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

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
void trigger_show_helper_message(int help);
void show_stat(int init, int end, int win);
void trigger_stats(void);
int win_refresh_and_getch(void);
void print_info(const char *str, int i);
void ask_user(const char *str, char *input, int dim, char c);
void resize_win(int help);
void change_sort(void);

#endif